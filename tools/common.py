"""도구 공통 코드: 설정 읽기, Object Storage(S3 호환) 연결, DB 연결.

설정은 프로젝트 폴더의 .env 파일 또는 환경 변수에서 읽는다. (.env.example 참고)
"""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def load_env():
    """.env 파일을 읽어 환경 변수로 넣는다. 이미 설정된 환경 변수가 우선한다."""
    env_file = Path(os.environ.get("OTT_ENV_FILE", ROOT / ".env"))
    if env_file.exists():
        for line in env_file.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            os.environ.setdefault(key.strip(), value.strip().strip('"').strip("'"))


def cfg(name, default=None, required=False):
    value = os.environ.get(name, default)
    if required and not value:
        sys.exit(f"[설정 오류] {name} 값이 없습니다. .env 파일을 확인하세요.")
    return value


load_env()
BUCKET = cfg("NCP_BUCKET", "acme")


def s3_client():
    """NCP Object Storage 클라이언트. AWS S3와 같은 방식(S3 호환 API)으로 접근한다."""
    import boto3
    from botocore.config import Config

    options = dict(
        signature_version="s3v4",
        s3={"addressing_style": "path"},          # https://엔드포인트/버킷/키 형식
        retries={"max_attempts": 5, "mode": "standard"},
    )
    try:
        # 최신 boto3는 업로드마다 CRC 체크섬을 붙이는데, S3 호환 저장소가 거절할 수 있어 끈다.
        config = Config(request_checksum_calculation="when_required",
                        response_checksum_validation="when_required", **options)
    except TypeError:                             # 구버전 boto3에는 이 옵션이 없음
        config = Config(**options)
    return boto3.client(
        "s3",
        endpoint_url=cfg("NCP_ENDPOINT", "https://kr.object.ncloudstorage.com"),
        region_name=cfg("NCP_REGION", "kr-standard"),
        aws_access_key_id=cfg("NCP_ACCESS_KEY", required=True),
        aws_secret_access_key=cfg("NCP_SECRET_KEY", required=True),
        config=config,
    )


def db_connect():
    import psycopg2

    return psycopg2.connect(
        host=cfg("DB_HOST", "localhost"), port=cfg("DB_PORT", "5432"),
        dbname=cfg("DB_NAME", "ott"), user=cfg("DB_USER", "ott"),
        password=cfg("DB_PASSWORD", ""),
    )


def human(num_bytes):
    for unit in ("B", "KB", "MB", "GB", "TB"):
        if abs(num_bytes) < 1024:
            return f"{num_bytes:,.1f} {unit}"
        num_bytes /= 1024
    return f"{num_bytes:,.1f} PB"
