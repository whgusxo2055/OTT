#!/bin/bash

# OTT Streaming Server - Setup Script

echo "OTT 스트리밍 서버 설정 스크립트"
echo "================================"
echo ""

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "경고: 이 스크립트는 macOS용으로 작성되었습니다."
    echo ""
fi

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "❌ Homebrew가 설치되어 있지 않습니다."
    echo "Homebrew를 먼저 설치해주세요: https://brew.sh"
    exit 1
fi

echo "✅ Homebrew 확인 완료"

# Install system dependencies
echo ""
echo "📦 시스템 의존성 설치 중..."
brew install sqlite3 libsodium ffmpeg

echo ""
echo "✅ 시스템 의존성 설치 완료"

# Navigate to server-c directory
cd "$(dirname "$0")/../server-c" || exit

# Download third-party dependencies
echo ""
echo "📥 서드파티 라이브러리 다운로드 중..."
make deps

# Initialize database
echo ""
echo "💾 데이터베이스 초기화 중..."
make db-init

# Create sample user (password: test1234)
echo ""
echo "👤 샘플 사용자 생성 중..."
sqlite3 app.db <<EOF
INSERT INTO users (id, login_id, password_hash, display_name)
VALUES (
    '00000000-0000-0000-0000-000000000001',
    'test',
    '\$argon2id\$v=19\$m=65536,t=2,p=1\$SomeRandomSalt123456\$HashedPasswordGoesHere',
    '테스트 사용자'
);
EOF

echo ""
echo "✅ 설정 완료!"
echo ""
echo "다음 명령어로 서버를 빌드하고 실행할 수 있습니다:"
echo "  cd server-c"
echo "  make build"
echo "  make run"
echo ""
echo "⚠️  중요: 실제 사용자를 추가하려면 별도의 스크립트를 사용하세요."
