#!/usr/bin/env python3
"""운영 명령 모음.

  python3 tools/manage.py check           # 버킷·DB 연결 확인 (가장 먼저 실행)
  python3 tools/manage.py status          # 진행 상황 (등록·인코딩·공개 개수, 버킷 사용량)
  python3 tools/manage.py publish-ready   # 인코딩이 끝난 영화를 한꺼번에 공개
  python3 tools/manage.py set-cors        # 버킷에 브라우저 CORS 설정
  python3 tools/manage.py get-cors        # 현재 CORS 설정 보기
  python3 tools/manage.py retry-failed    # 실패한 인코딩 작업을 다시 대기열로
"""
import argparse
import base64
import hashlib
import sys
from pathlib import Path

from common import BUCKET, cfg, db_connect, human, s3_client


def cmd_check(_):
    print(f"[1/2] Object Storage  엔드포인트 {cfg('NCP_ENDPOINT', 'https://kr.object.ncloudstorage.com')}  버킷 {BUCKET}")
    s3 = s3_client()
    try:
        key = "healthcheck/ott-cms-check.txt"
        s3.put_object(Bucket=BUCKET, Key=key, Body=b"ok", ContentType="text/plain")
        body = s3.get_object(Bucket=BUCKET, Key=key)["Body"].read()
        s3.delete_object(Bucket=BUCKET, Key=key)
        print("      OK: 쓰기, 읽기, 삭제" if body == b"ok" else "      읽은 내용이 다릅니다")
    except Exception as e:
        print(f"      실패: {type(e).__name__}: {e}")
        print("      → Access Key / Secret Key, 버킷 이름, 엔드포인트를 확인하세요.")
        print("        (NCP 새 형식 키는 Access Key가 ncp_iam_BPAS..., Secret Key가 ncp_iam_BPKS... 로 서로 다릅니다)")
        sys.exit(1)
    print(f"[2/2] DB  {cfg('DB_HOST', 'localhost')}:{cfg('DB_PORT', '5432')}/{cfg('DB_NAME', 'ott')}")
    try:
        conn = db_connect()
        with conn, conn.cursor() as cur:
            cur.execute("SELECT count(*) FROM content")
            print(f"      OK: content {cur.fetchone()[0]}행")
    except Exception as e:
        print(f"      실패: {type(e).__name__}: {e}")
        print("      → DB 접속 정보와 sql/schema.sql 적용 여부를 확인하세요.")
        sys.exit(1)


def bucket_usage(s3):
    """접두사별 객체 수와 용량. (NCP는 목록 조회 V1만 지원)"""
    usage, marker = {}, ""
    while True:
        page = s3.list_objects(Bucket=BUCKET, Marker=marker, MaxKeys=1000)
        items = page.get("Contents", [])
        for obj in items:
            top = obj["Key"].split("/", 1)[0] + "/"
            count, size = usage.get(top, (0, 0))
            usage[top] = (count + 1, size + obj["Size"])
        if not page.get("IsTruncated") or not items:
            break
        marker = page.get("NextMarker") or items[-1]["Key"]
    return usage


def cmd_status(args):
    conn = db_connect()
    with conn, conn.cursor() as cur:
        cur.execute("SELECT status::text, count(*) FROM encoding_job GROUP BY status ORDER BY status")
        jobs = dict(cur.fetchall())
        cur.execute("SELECT publish_status::text, count(*) FROM content WHERE deleted_at IS NULL GROUP BY 1")
        pub = dict(cur.fetchall())
        cur.execute("SELECT count(*) FROM available_movie")
        available = cur.fetchone()[0]
        cur.execute("""SELECT height, count(*), COALESCE(sum(duration_sec), 0), COALESCE(sum(size_bytes), 0)
                       FROM media_asset WHERE asset_type = 'SOURCE_VIDEO' GROUP BY height ORDER BY height""")
        by_res = cur.fetchall()
        cur.execute("SELECT error_message, count(*) FROM encoding_job WHERE status = 'FAILED' GROUP BY 1 LIMIT 5")
        errors = cur.fetchall()
    print("인코딩 작업   " + "  ".join(f"{k} {jobs.get(k, 0)}" for k in ("WAITING", "PROCESSING", "DONE", "FAILED")))
    print(f"콘텐츠        비공개 {pub.get('PRIVATE', 0)}  공개 {pub.get('PUBLIC', 0)}")
    print(f"지금 사용자에게 보이는 영화  {available}개  (공개 + 인코딩 완료 + 판권 기간 안)")
    print("원본 해상도별")
    for height, count, seconds, size in by_res:
        label = "4K" if height == 2160 else f"{height}p" if height else "확인 전"
        print(f"  {label:<6}{count:>6}개  {float(seconds) / 3600:>6.1f}시간  {human(int(size))}")
    for message, count in errors:
        print(f"  실패 {count}건: {message[:120]}")
    if args.bucket:
        print("버킷 사용량 (객체 수가 많으면 시간이 걸립니다)")
        total = 0
        for prefix, (count, size) in sorted(bucket_usage(s3_client()).items()):
            total += size
            print(f"  {prefix:<12}{count:>8}개  {human(size)}")
        print(f"  {'합계':<10}{'':>10}{human(total)}")


def cmd_publish_ready(_):
    conn = db_connect()
    with conn, conn.cursor() as cur:
        cur.execute((Path(__file__).resolve().parent.parent / "sql" / "publish_ready.sql").read_text(encoding="utf-8"))
        print(f"{cur.rowcount}개를 공개했습니다.")
        cur.execute("SELECT count(*) FROM available_movie")
        print(f"지금 사용자에게 보이는 영화: {cur.fetchone()[0]}개")


def cmd_retry_failed(_):
    conn = db_connect()
    with conn, conn.cursor() as cur:
        cur.execute("""UPDATE encoding_job SET status = 'WAITING', worker_id = NULL, retry_count = retry_count + 1
                       WHERE status = 'FAILED'""")
        print(f"{cur.rowcount}개를 다시 대기열로 돌렸습니다.")


def _add_content_md5(request, **_):
    """NCP는 CORS 설정 요청에 Content-MD5 헤더를 요구한다. 최신 boto3는 이 헤더를 붙이지 않으므로 직접 붙인다."""
    if "Content-MD5" not in request.headers:
        body = request.body or b""
        if isinstance(body, str):
            body = body.encode("utf-8")
        request.headers["Content-MD5"] = base64.b64encode(hashlib.md5(body).digest()).decode()


def cmd_set_cors(args):
    s3 = s3_client()
    s3.meta.events.register("before-sign.s3.PutBucketCors", _add_content_md5)
    rule = {"AllowedOrigins": args.origin, "AllowedMethods": ["GET", "HEAD"],
            "AllowedHeaders": ["*"], "ExposeHeaders": ["Content-Length", "Content-Range", "ETag"],
            "MaxAgeSeconds": 3600}
    try:
        s3.put_bucket_cors(Bucket=BUCKET, CORSConfiguration={"CORSRules": [rule]})
        print(f"CORS 설정 완료: 허용 출처 {args.origin}, 메서드 GET/HEAD")
    except Exception as e:
        print(f"CORS 설정 실패: {type(e).__name__}: {e}")
        print("→ 이 API 키에 버킷 설정 권한이 없을 수 있습니다. 그 경우 CMS 서버의 .env에서")
        print("  스트리밍 서버를 통한 HLS 재생에는 버킷 CORS가 필요하지 않습니다.")
        sys.exit(1)


def cmd_get_cors(_):
    try:
        for rule in s3_client().get_bucket_cors(Bucket=BUCKET)["CORSRules"]:
            print(rule)
    except Exception as e:
        print(f"CORS 설정이 없거나 조회할 수 없습니다: {type(e).__name__}: {e}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)
    sub.add_parser("check").set_defaults(func=cmd_check)
    p = sub.add_parser("status")
    p.add_argument("--bucket", action="store_true", help="버킷 사용량도 함께 계산")
    p.set_defaults(func=cmd_status)
    sub.add_parser("publish-ready").set_defaults(func=cmd_publish_ready)
    sub.add_parser("retry-failed").set_defaults(func=cmd_retry_failed)
    p = sub.add_parser("set-cors")
    p.add_argument("--origin", nargs="+", default=["*"], help="허용할 출처. 기본은 모두 허용(*)")
    p.set_defaults(func=cmd_set_cors)
    sub.add_parser("get-cors").set_defaults(func=cmd_get_cors)
    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
