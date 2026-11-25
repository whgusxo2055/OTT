#!/usr/bin/env python3
"""3단계: 인코딩 워커. 대기 중인 작업을 하나씩 가져와 원본 영상을 HLS로 바꿔 버킷에 올린다.

작업 하나를 처리하는 순서
  ① DB에서 WAITING 작업 하나를 잡아 PROCESSING으로 바꾼다 (다른 워커와 겹치지 않게 잠금)
  ② 버킷에서 원본을 내려받는다                  original/C00001.mp4
  ③ FFmpeg로 HLS(6초 조각 + 재생목록)로 만든다
  ④ 결과를 버킷에 올린다                        hls/{콘텐츠ID}/{작업ID}/index.m3u8, seg_00000.ts ...
  ⑤ DB에 DONE + 결과 위치를 기록하고, 콘텐츠의 active_job_id를 이 작업으로 바꾼다
  실패하면 FAILED + 오류 내용을 기록한다 (기존에 재생되던 영상은 그대로 유지)

원본이 이미 H.264 + AAC면 다시 압축하지 않고 조각만 낸다(빠름). 다른 코덱(MOV 등)이면 변환한다.
워커를 여러 개 띄우려면 이 명령을 여러 터미널에서 실행하면 된다.

예)  python3 tools/worker.py            # 계속 대기하며 처리
     python3 tools/worker.py --once     # 대기 작업을 다 처리하면 종료
"""
import argparse
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from common import BUCKET, db_connect, s3_client

SEGMENT_SECONDS = 6
CONTENT_TYPES = {".m3u8": "application/vnd.apple.mpegurl", ".ts": "video/mp2t"}

CLAIM_SQL = """
UPDATE encoding_job
SET status = 'PROCESSING', worker_id = %s, started_at = now(), error_message = NULL
WHERE id = (SELECT id FROM encoding_job WHERE status = 'WAITING'
            ORDER BY created_at LIMIT 1
            FOR UPDATE SKIP LOCKED)          -- 다른 워커가 잡은 작업은 건너뛴다
RETURNING id, source_asset_id
"""


def claim_job(conn, worker_id):
    with conn, conn.cursor() as cur:
        cur.execute(CLAIM_SQL, (worker_id,))
        job = cur.fetchone()
        if not job:
            return None
        cur.execute("SELECT content_id, storage_key FROM media_asset WHERE id = %s", (job[1],))
        content_id, source_key = cur.fetchone()
    return dict(id=str(job[0]), asset_id=str(job[1]), content_id=str(content_id), source_key=source_key)


def recover_stale(conn, minutes):
    """워커가 죽어서 PROCESSING에 멈춘 작업을 다시 대기열로 돌린다."""
    with conn, conn.cursor() as cur:
        cur.execute("""UPDATE encoding_job
                       SET status = 'WAITING', worker_id = NULL, retry_count = retry_count + 1
                       WHERE status = 'PROCESSING' AND started_at < now() - make_interval(mins => %s)""",
                    (minutes,))
        if cur.rowcount:
            print(f"멈춰 있던 작업 {cur.rowcount}개를 다시 대기열로 돌렸습니다.", flush=True)


def probe(path):
    done = subprocess.run(["ffprobe", "-v", "error", "-show_entries",
                           "stream=codec_type,codec_name,width,height:format=duration", "-of", "json", str(path)],
                          capture_output=True, text=True)
    if done.returncode != 0:
        raise RuntimeError("영상 파일을 읽을 수 없습니다: " + done.stderr.strip()[-300:])
    info = json.loads(done.stdout)
    video = next((s for s in info["streams"] if s["codec_type"] == "video"), None)
    audio = next((s for s in info["streams"] if s["codec_type"] == "audio"), None)
    if not video:
        raise RuntimeError("영상 트랙이 없는 파일입니다.")
    return dict(vcodec=video["codec_name"], acodec=audio["codec_name"] if audio else None,
                width=int(video["width"]), height=int(video["height"]),
                duration=round(float(info["format"]["duration"]), 2))


def make_hls(source, out_dir, info, force_transcode=False):
    """HLS 만들기. 결과: out_dir/index.m3u8 + seg_00000.ts ..."""
    can_copy = info["vcodec"] == "h264" and info["acodec"] in ("aac", None) and not force_transcode
    if can_copy:
        codec = ["-c", "copy"]                              # 재압축 없이 조각만 낸다
    else:
        codec = ["-c:v", "libx264", "-preset", "veryfast", "-crf", "23", "-pix_fmt", "yuv420p",
                 "-force_key_frames", f"expr:gte(t,n_forced*{SEGMENT_SECONDS})",
                 "-c:a", "aac", "-b:a", "128k"]
    cmd = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", str(source), *codec,
           "-f", "hls", "-hls_time", str(SEGMENT_SECONDS), "-hls_playlist_type", "vod",
           "-hls_segment_filename", str(out_dir / "seg_%05d.ts"), str(out_dir / "index.m3u8")]
    done = subprocess.run(cmd, capture_output=True, text=True)
    if done.returncode != 0:
        raise RuntimeError("ffmpeg 실패: " + done.stderr.strip()[-500:])
    return "copy" if can_copy else "transcode"


def process(conn, s3, job, force_transcode):
    workdir = Path(tempfile.mkdtemp(prefix="ott-worker-"))
    try:
        source = workdir / ("source" + Path(job["source_key"]).suffix)
        s3.download_file(BUCKET, job["source_key"], str(source))
        info = probe(source)
        out_dir = workdir / "hls"
        out_dir.mkdir()
        mode = make_hls(source, out_dir, info, force_transcode)

        prefix = f"hls/{job['content_id']}/{job['id']}/"
        files = sorted(out_dir.iterdir(), key=lambda p: p.suffix == ".m3u8")   # 재생목록은 맨 마지막에 올린다
        total = 0
        for f in files:
            s3.upload_file(str(f), BUCKET, prefix + f.name,
                           ExtraArgs={"ContentType": CONTENT_TYPES.get(f.suffix, "application/octet-stream")})
            total += f.stat().st_size

        with conn, conn.cursor() as cur:        # 한 트랜잭션: 작업 완료 + 재생 영상 교체 + 원본 정보 보정
            cur.execute("""UPDATE encoding_job SET status = 'DONE', output_key = %s, finished_at = now()
                           WHERE id = %s""", (prefix + "index.m3u8", job["id"]))
            cur.execute("UPDATE content SET active_job_id = %s, updated_at = now() WHERE id = %s",
                        (job["id"], job["content_id"]))
            cur.execute("""UPDATE media_asset SET width = %s, height = %s, duration_sec = %s
                           WHERE id = %s""", (info["width"], info["height"], info["duration"], job["asset_id"]))
        return f"{info['height']}p {info['duration']:.0f}초 {mode} 조각 {len(files) - 1}개 {total / 1e6:.1f}MB"
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


def mark_failed(conn, job_id, message):
    conn.rollback()
    with conn, conn.cursor() as cur:
        cur.execute("""UPDATE encoding_job SET status = 'FAILED', error_message = %s, finished_at = now()
                       WHERE id = %s""", (message[:1000], job_id))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--once", action="store_true", help="대기 작업이 없으면 종료")
    ap.add_argument("--poll", type=int, default=5, help="대기 작업이 없을 때 다시 확인하는 간격(초)")
    ap.add_argument("--stale-minutes", type=int, default=30, help="이 시간 넘게 처리 중이면 멈춘 작업으로 본다")
    ap.add_argument("--transcode", action="store_true", help="항상 다시 압축한다 (기본: 가능하면 조각만 냄)")
    ap.add_argument("--worker-id", default=f"{socket.gethostname()}-{os.getpid()}")
    args = ap.parse_args()

    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        sys.exit("ffmpeg / ffprobe가 필요합니다.  (Ubuntu: sudo apt install -y ffmpeg)")
    conn, s3 = db_connect(), s3_client()
    print(f"워커 시작: {args.worker_id}", flush=True)
    done = failed = 0
    last_recover = 0.0
    while True:
        if time.time() - last_recover > 60:
            recover_stale(conn, args.stale_minutes)
            last_recover = time.time()
        job = claim_job(conn, args.worker_id)
        if not job:
            if args.once:
                break
            time.sleep(args.poll)
            continue
        started = time.time()
        try:
            summary = process(conn, s3, job, args.transcode)
            done += 1
            print(f"  [완료 {done}] {job['source_key']}  {summary}  {time.time() - started:.1f}초", flush=True)
        except Exception as e:
            failed += 1
            mark_failed(conn, job["id"], f"{type(e).__name__}: {e}")
            print(f"  [실패] {job['source_key']}: {type(e).__name__}: {e}", flush=True)
    print(f"워커 종료: 완료 {done}개, 실패 {failed}개")


if __name__ == "__main__":
    main()
