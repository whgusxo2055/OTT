#!/usr/bin/env python3
"""2단계: 수급(입수). 계획표(metadata.csv)대로 영상을 만들어 Object Storage에 올리고 CMS DB에 등록한다.

한 편마다 하는 일
  ① FFmpeg로 영상 생성 (수급처가 보내 준 원본 영상 역할)
  ② 버킷에 업로드:  original/C00001.mp4 , poster/C00001.svg
  ③ CMS DB 등록:    content(비공개) + media_asset(파일 위치) + encoding_job(WAITING)
  ④ 로컬 파일 삭제 (서버 디스크를 채우지 않음)

중간에 멈춰도 다시 실행하면 이미 등록된 영상은 건너뛴다. (source, external_id 중복 금지 제약 덕분)

예)  python3 tools/ingest.py --csv metadata.csv --limit 20      # 먼저 20개만 시험
     python3 tools/ingest.py --csv metadata.csv                 # 전체
"""
import argparse
import colorsys
import csv
import json
import os
import shutil
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from common import BUCKET, db_connect, human, s3_client

SOURCE = "SYNTHETIC"          # 수급처 이름: 자체 생성 데이터
FPS = 24
lock = threading.Lock()


def find_font():
    """영상에 콘텐츠 ID를 찍을 글꼴. 없으면 글자 없이 만든다."""
    try:
        path = subprocess.run(["fc-match", "-f", "%{file}", "DejaVu Sans:bold"],
                              capture_output=True, text=True, timeout=10).stdout.strip()
        if path and Path(path).exists():
            return path
    except Exception:
        pass
    for path in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                 "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf",
                 "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf"):
        if Path(path).exists():
            return path
    return None


def make_video(row, index, out_path, font):
    """테스트 패턴 영상 생성. 영상마다 색과 소리가 다르고, 화면에 콘텐츠 ID와 해상도가 찍힌다."""
    w, h, dur = int(row["width"]), int(row["height"]), int(row["duration_sec"])
    hue = (index * 37) % 360
    freq = 220 + (index * 13) % 660
    vf = f"hue=h={hue}"
    if font:
        label = f"{row['cid']}  {h}p"
        size = h // 9
        vf += (f",drawtext=fontfile={font}:text='{label}':fontsize={size}:fontcolor=white:"
               f"box=1:boxcolor=black@0.5:boxborderw={size // 4}:x=(w-tw)/2:y=h-{size * 2}")
    cmd = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
           "-f", "lavfi", "-i", f"testsrc2=size={w}x{h}:rate={FPS}:duration={dur}",
           "-f", "lavfi", "-i", f"sine=frequency={freq}:duration={dur}",
           "-vf", vf,
           "-c:v", "libx264", "-preset", "veryfast", "-crf", "28", "-pix_fmt", "yuv420p",
           # 2초마다 키프레임 → 나중에 HLS로 자를 때 재인코딩 없이 깔끔하게 잘린다
           "-g", str(FPS * 2), "-keyint_min", str(FPS * 2), "-sc_threshold", "0",
           "-c:a", "aac", "-b:a", "96k",
           "-movflags", "+faststart", "-shortest", str(out_path)]
    subprocess.run(cmd, check=True, capture_output=True, text=True)


def probe(path):
    """ffprobe로 실제 해상도와 길이를 확인한다. (받은 파일을 믿지 않고 직접 확인)"""
    out = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0",
                          "-show_entries", "stream=width,height:format=duration",
                          "-of", "json", str(path)], check=True, capture_output=True, text=True).stdout
    info = json.loads(out)
    stream = info["streams"][0]
    return int(stream["width"]), int(stream["height"]), round(float(info["format"]["duration"]), 2)


def hex_color(h, s, v):
    r, g, b = colorsys.hsv_to_rgb(h % 1.0, s, v)
    return f"#{int(r * 255):02X}{int(g * 255):02X}{int(b * 255):02X}"


def make_poster_svg(index):
    """목업 아트워크와 같은 구도의 16:9 포스터(SVG). 작품마다 색과 해 위치가 다르다."""
    hue = (index * 0.137) % 1.0
    dark, light = hex_color(hue, 0.55, 0.26), hex_color(hue + 0.06, 0.42, 0.62)
    cx, cy, r = 740 + (index * 53) % 240, 180 + (index * 29) % 80, 70 + (index * 17) % 70
    return (f'<svg width="1280" height="720" viewBox="0 0 1280 720" xmlns="http://www.w3.org/2000/svg"><defs>'
            f'<linearGradient id="g" x2="1" y2="1"><stop stop-color="{dark}"/><stop offset="1" stop-color="{light}"/></linearGradient>'
            f'<linearGradient id="fade" x2="0" y2="1"><stop stop-color="#10131E" stop-opacity="0"/>'
            f'<stop offset="1" stop-color="#10131E" stop-opacity=".72"/></linearGradient></defs>'
            f'<rect width="1280" height="720" fill="url(#g)"/><circle cx="{cx}" cy="{cy}" r="{r}" fill="#F6E5BD" opacity=".5"/>'
            f'<path d="M0 550L260 330L550 560L820 360L1280 600V720H0Z" fill="{dark}" opacity=".7"/>'
            f'<path d="M0 640L480 520L940 630L1280 450V720H0Z" fill="#10131E" opacity=".65"/>'
            f'<path d="M630 720L780 500L790 500L740 720Z" fill="#F6E5BD" opacity=".15"/>'
            f'<rect width="1280" height="720" fill="url(#fade)"/></svg>')


def register(conn, row, video_key, poster_key, video_size, poster_size, width, height, duration):
    """CMS DB 등록. 한 트랜잭션으로 처리해서 일부만 들어가는 일이 없게 한다."""
    with conn, conn.cursor() as cur:
        cur.execute("INSERT INTO genre(name) VALUES (%s) ON CONFLICT (name) DO NOTHING", (row["genre"],))
        cur.execute("SELECT id FROM genre WHERE name = %s", (row["genre"],))
        genre_id = cur.fetchone()[0]
        cur.execute("""
            INSERT INTO content (title, description, genre_id, release_year, age_rating, actors, tags,
                                 runtime_sec, source, external_id, license_start, license_end)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            ON CONFLICT (source, external_id) DO NOTHING
            RETURNING id""",
            (row["title"], row["description"], genre_id, int(row["release_year"]), row["age_rating"],
             row["actors"], row["tags"], max(1, round(duration)), SOURCE, row["cid"],
             row["license_start"], row["license_end"]))
        got = cur.fetchone()
        if not got:                      # 이미 등록된 콘텐츠
            return False
        content_id = got[0]
        cur.execute("""INSERT INTO media_asset (content_id, asset_type, storage_key, mime_type, size_bytes,
                                                width, height, duration_sec)
                       VALUES (%s, 'SOURCE_VIDEO', %s, 'video/mp4', %s, %s, %s, %s) RETURNING id""",
                    (content_id, video_key, video_size, width, height, duration))
        video_asset_id = cur.fetchone()[0]
        cur.execute("""INSERT INTO media_asset (content_id, asset_type, storage_key, mime_type, size_bytes)
                       VALUES (%s, 'POSTER', %s, 'image/svg+xml', %s) RETURNING id""",
                    (content_id, poster_key, poster_size))
        cur.execute("UPDATE content SET poster_asset_id = %s WHERE id = %s", (cur.fetchone()[0], content_id))
        # 원본 등록 완료 → 인코딩 대기열에 넣는다 (워커가 가져간다)
        cur.execute("INSERT INTO encoding_job (source_asset_id) VALUES (%s)", (video_asset_id,))
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--csv", default="metadata.csv")
    ap.add_argument("--limit", type=int, help="이번 실행에서 새로 처리할 최대 개수")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2), help="동시에 만들 영상 수")
    ap.add_argument("--max-gb", type=float, default=19.0, help="원본 합계가 이 용량을 넘으면 멈춘다 (안전장치). HLS 변환본까지 합치면 약 2배가 된다")
    ap.add_argument("--workdir", default="/tmp/ott-ingest")
    args = ap.parse_args()

    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        sys.exit("ffmpeg / ffprobe가 필요합니다.  (Ubuntu: sudo apt install -y ffmpeg)")
    rows = list(csv.DictReader(open(args.csv, encoding="utf-8-sig")))
    workdir = Path(args.workdir)
    workdir.mkdir(parents=True, exist_ok=True)
    font = find_font()
    if not font:
        print("※ 글꼴을 찾지 못해 영상에 콘텐츠 ID를 찍지 않습니다. (sudo apt install -y fonts-dejavu-core)")

    conn = db_connect()
    with conn, conn.cursor() as cur:
        cur.execute("SELECT external_id FROM content WHERE source = %s", (SOURCE,))
        done = {r[0] for r in cur.fetchall()}
        cur.execute("SELECT COALESCE(sum(size_bytes), 0) FROM media_asset WHERE asset_type = 'SOURCE_VIDEO'")
        used = int(cur.fetchone()[0])
    conn.close()

    todo = [(i, r) for i, r in enumerate(rows) if r["cid"] not in done]
    if args.limit:
        todo = todo[:args.limit]
    print(f"계획 {len(rows)}개 중 이미 등록 {len(done)}개, 이번에 처리 {len(todo)}개 "
          f"(동시 {args.jobs}개, 현재 원본 합계 {human(used)})")
    if not todo:
        return

    s3 = s3_client()
    state = dict(used=used, ok=0, fail=0, stop=False)
    max_bytes = args.max_gb * 1024 ** 3
    started = time.time()
    local = threading.local()

    def work(item):
        index, row = item
        if state["stop"]:
            return
        cid = row["cid"]
        path = workdir / f"{cid}.mp4"
        try:
            make_video(row, index, path, font)
            width, height, duration = probe(path)
            size = path.stat().st_size
            with lock:
                if state["used"] + size > max_bytes:
                    state["stop"] = True
                    print(f"\n[중단] 원본 합계가 한도({args.max_gb} GB)를 넘게 되어 멈춥니다.")
                    return
                state["used"] += size
            video_key, poster_key = f"original/{cid}.mp4", f"poster/{cid}.svg"
            poster = make_poster_svg(index).encode("utf-8")
            s3.upload_file(str(path), BUCKET, video_key, ExtraArgs={"ContentType": "video/mp4"})
            s3.put_object(Bucket=BUCKET, Key=poster_key, Body=poster, ContentType="image/svg+xml")
            if not getattr(local, "conn", None):
                local.conn = db_connect()
            register(local.conn, row, video_key, poster_key, size, len(poster), width, height, duration)
            with lock:
                state["ok"] += 1
                n = state["ok"]
            if n % 10 == 0 or n == len(todo):
                elapsed = time.time() - started
                remain = (len(todo) - n) * elapsed / n
                print(f"  {n}/{len(todo)}  원본 합계 {human(state['used'])}  "
                      f"경과 {elapsed / 60:.0f}분  남은 시간 약 {remain / 60:.0f}분", flush=True)
        except subprocess.CalledProcessError as e:
            with lock:
                state["fail"] += 1
            print(f"  [실패] {cid}: ffmpeg 오류 {e.stderr[-300:] if e.stderr else e}", flush=True)
        except Exception as e:
            with lock:
                state["fail"] += 1
            print(f"  [실패] {cid}: {type(e).__name__}: {e}", flush=True)
        finally:
            path.unlink(missing_ok=True)

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for _ in as_completed([pool.submit(work, item) for item in todo]):
            pass

    print(f"\n완료: 성공 {state['ok']}개, 실패 {state['fail']}개, 원본 합계 {human(state['used'])}")
    if state["fail"]:
        print("실패한 영상은 같은 명령을 다시 실행하면 그것만 다시 시도합니다.")


if __name__ == "__main__":
    main()
