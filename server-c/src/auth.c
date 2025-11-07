#include <string.h>
#include <sodium.h>
#include "auth.h"
#include "logger.h"
#include "db.h"

// Base64 디코드 테이블
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int auth_hash_password(const char *password, char *hash_out, size_t hash_len) {
    if (sodium_init() < 0) {
        log_error("libsodium 초기화 실패");
        return -1;
    }

    if (hash_len < crypto_pwhash_STRBYTES) {
        log_error("해시 버퍼가 너무 작습니다");
        return -1;
    }

    if (crypto_pwhash_str(hash_out, password, strlen(password),
                         crypto_pwhash_OPSLIMIT_INTERACTIVE,
                         crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        log_error("비밀번호 해싱 실패 (메모리 부족)");
        return -1;
    }

    return 0;
}

bool auth_verify_password(const char *password, const char *hash) {
    if (sodium_init() < 0) {
        log_error("libsodium 초기화 실패");
        return false;
    }

    if (crypto_pwhash_str_verify(hash, password, strlen(password)) != 0) {
        return false;
    }

    return true;
}

static int base64_decode(const char *input, char *output, size_t output_len) {
    size_t input_len = strlen(input);
    size_t output_pos = 0;
    unsigned int bits = 0;
    int bit_count = 0;

    for (size_t i = 0; i < input_len; i++) {
        char c = input[i];
        if (c == '=') break;

        const char *pos = strchr(base64_table, c);
        if (pos == NULL) continue;

        bits = (bits << 6) | (pos - base64_table);
        bit_count += 6;

        if (bit_count >= 8) {
            bit_count -= 8;
            if (output_pos >= output_len - 1) return -1;
            output[output_pos++] = (bits >> bit_count) & 0xFF;
        }
    }

    output[output_pos] = '\0';
    return output_pos;
}

int auth_parse_basic_header(const char *auth_header, char *username, char *password, size_t len) {
    if (auth_header == NULL || username == NULL || password == NULL) {
        return -1;
    }

    // "Basic " 접두사 확인
    if (strncmp(auth_header, "Basic ", 6) != 0) {
        log_warn("잘못된 인증 헤더 형식");
        return -1;
    }

    // base64 디코드
    char decoded[512];
    if (base64_decode(auth_header + 6, decoded, sizeof(decoded)) < 0) {
        log_warn("base64 인증 헤더 디코드 실패");
        return -1;
    }

    // username:password 분리
    char *colon = strchr(decoded, ':');
    if (colon == NULL) {
        log_warn("잘못된 자격 증명 형식");
        return -1;
    }

    size_t username_len = colon - decoded;
    size_t password_len = strlen(colon + 1);

    if (username_len >= len || password_len >= len) {
        log_warn("자격 증명이 너무 깁니다");
        return -1;
    }

    strncpy(username, decoded, username_len);
    username[username_len] = '\0';
    strcpy(password, colon + 1);

    return 0;
}

int auth_authenticate_user(const char *auth_header, user_t *user) {
    if (auth_header == NULL || user == NULL) {
        return -1;
    }

    char username[64];
    char password[128];

    if (auth_parse_basic_header(auth_header, username, password, sizeof(username)) < 0) {
        return -1;
    }

    // 데이터베이스에서 사용자 가져오기
    if (db_get_user_by_login(username, user) < 0) {
        log_warn("사용자를 찾을 수 없음: %s", username);
        return -1;
    }

    // 비밀번호 확인
    if (!auth_verify_password(password, user->password_hash)) {
        log_warn("사용자의 비밀번호가 올바르지 않음: %s", username);
        return -1;
    }

    log_info("사용자 인증 성공: %s", username);
    return 0;
}
