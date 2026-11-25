#!/usr/bin/env bash
# NCP 서버(Ubuntu 22.04 / 24.04) 한 번만 실행:  sudo bash scripts/setup_server.sh
# 하는 일: 필요한 프로그램 설치 → PostgreSQL에 DB·계정 생성 → 스키마 적용
set -euo pipefail
cd "$(dirname "$0")/.."

[ -f .env ] || { echo ".env 파일이 없습니다. 먼저  cp .env.example .env  후 값을 채우세요."; exit 1; }
get() { grep -E "^$1=" .env | head -1 | cut -d= -f2- | tr -d '\r'; }
DB_NAME=$(get DB_NAME); DB_USER=$(get DB_USER); DB_PASSWORD=$(get DB_PASSWORD)
case "$DB_PASSWORD" in ""|여기에*) echo ".env 의 DB_PASSWORD 를 정해 주세요."; exit 1;; esac
case "$DB_PASSWORD" in *[!A-Za-z0-9_-]*) echo "DB_PASSWORD 는 영문·숫자·_·- 만 사용해 주세요."; exit 1;; esac

echo "== 1/3 프로그램 설치 =="
export DEBIAN_FRONTEND=noninteractive
apt-get update -y
apt-get install -y ffmpeg fonts-dejavu-core postgresql python3 python3-boto3 python3-psycopg2 tmux unzip curl
# CMS API(Spring Boot) 빌드·실행용. 실패해도 영상 도구는 쓸 수 있으므로 계속 진행한다.
apt-get install -y openjdk-21-jdk-headless maven || echo "※ Java 21 / Maven 설치 실패: OS 버전을 확인하세요. (Ubuntu 22.04 이상 필요)"

echo "== 2/3 DB 생성 =="
systemctl enable --now postgresql
sudo -u postgres psql -v ON_ERROR_STOP=1 -q <<SQL
DO \$\$ BEGIN
  IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = '$DB_USER') THEN
    CREATE ROLE "$DB_USER" LOGIN PASSWORD '$DB_PASSWORD';
  ELSE
    ALTER ROLE "$DB_USER" PASSWORD '$DB_PASSWORD';
  END IF;
END \$\$;
SQL
sudo -u postgres psql -tAc "SELECT 1 FROM pg_database WHERE datname = '$DB_NAME'" | grep -q 1 \
  || sudo -u postgres createdb -O "$DB_USER" "$DB_NAME"

echo "== 3/3 스키마 적용 =="
if PGPASSWORD="$DB_PASSWORD" psql -h 127.0.0.1 -U "$DB_USER" -d "$DB_NAME" -tAc "SELECT to_regclass('content')" | grep -q content; then
  echo "이미 테이블이 있어 건너뜁니다."
else
  PGPASSWORD="$DB_PASSWORD" psql -h 127.0.0.1 -U "$DB_USER" -d "$DB_NAME" -v ON_ERROR_STOP=1 -q -f sql/schema.sql
  echo "스키마 적용 완료"
fi

chmod 600 .env
echo
echo "준비 완료. 다음:  python3 tools/manage.py check"
