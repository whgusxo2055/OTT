#!/usr/bin/env bash
# CMS API 서버 빌드 + 실행:  cd cms && ./run.sh
# 설정은 상위 폴더의 .env 에서 읽는다. 처음 빌드할 때는 라이브러리를 내려받느라 몇 분 걸린다.
set -euo pipefail
cd "$(dirname "$0")"

[ -f ../.env ] || { echo "../.env 파일이 없습니다."; exit 1; }
# .env 를 환경 변수로 넣는다. (셸 특수문자를 해석하지 않도록 한 줄씩 직접 읽는다)
while IFS='=' read -r key value || [ -n "$key" ]; do
  key="${key%$'\r'}"
  [[ "$key" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || continue     # 빈 줄·주석은 건너뛴다
  export "$key=${value%$'\r'}"
done < ../.env
export MOCKUP_DIR="${MOCKUP_DIR:-$(cd ../mockup/dist && pwd)}"

../streaming-server/mvnw -q -DskipTests package
echo "CMS API 시작: http://<서버 주소>:${PORT:-8080}/   (목업: $MOCKUP_DIR)"
exec java -jar target/cms.jar
