#!/bin/bash

# Create user script for OTT Streaming Server

if [ $# -lt 3 ]; then
    echo "사용법: $0 <login_id> <password> <display_name>"
    echo "예제: $0 user1 mypassword '사용자1'"
    exit 1
fi

LOGIN_ID="$1"
PASSWORD="$2"
DISPLAY_NAME="$3"

# Build create_user utility if needed
cd "$(dirname "$0")/../server-c" || exit

# Create a temporary C program to hash password
cat > /tmp/hash_password.c <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <password>\n", argv[0]);
        return 1;
    }
    
    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialize libsodium\n");
        return 1;
    }
    
    char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hash, argv[1], strlen(argv[1]),
                         crypto_pwhash_OPSLIMIT_INTERACTIVE,
                         crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        fprintf(stderr, "Password hashing failed\n");
        return 1;
    }
    
    printf("%s\n", hash);
    return 0;
}
EOF

# Compile the utility
clang -o /tmp/hash_password /tmp/hash_password.c -lsodium

if [ $? -ne 0 ]; then
    echo "❌ 비밀번호 해시 유틸리티 컴파일 실패"
    exit 1
fi

# Hash the password
HASHED_PASSWORD=$(/tmp/hash_password "$PASSWORD")

if [ $? -ne 0 ]; then
    echo "❌ 비밀번호 해싱 실패"
    rm -f /tmp/hash_password
    exit 1
fi

# Generate UUID
UUID=$(uuidgen | tr '[:upper:]' '[:lower:]')

# Insert user into database
sqlite3 app.db <<EOF
INSERT INTO users (id, login_id, password_hash, display_name)
VALUES ('$UUID', '$LOGIN_ID', '$HASHED_PASSWORD', '$DISPLAY_NAME');
EOF

if [ $? -eq 0 ]; then
    echo "✅ 사용자 생성 성공!"
    echo "   ID: $UUID"
    echo "   로그인 아이디: $LOGIN_ID"
    echo "   이름: $DISPLAY_NAME"
else
    echo "❌ 사용자 생성 실패 (이미 존재하는 아이디일 수 있습니다)"
fi

# Cleanup
rm -f /tmp/hash_password /tmp/hash_password.c
