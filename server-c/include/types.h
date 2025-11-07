#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// UUID 타입 (36자 + null 종료문자)
typedef char ott_uuid_t[37];

// 사용자 구조체
typedef struct {
    ott_uuid_t id;
    char login_id[64];
    char password_hash[128];
    char display_name[128];
    time_t created_at;
    time_t updated_at;
} user_t;

// 동영상 메타데이터
typedef struct {
    ott_uuid_t id;
    char title[256];
    char description[1024];
    int duration_sec;
    char mime_type[64];
    time_t created_at;
    time_t updated_at;
} video_t;

// 동영상 파일
typedef struct {
    ott_uuid_t id;
    ott_uuid_t video_id;
    char file_path[512];
    int64_t file_size;
    int bitrate_kbps;
    char resolution[32];
    time_t created_at;
} video_file_t;

// 썸네일
typedef struct {
    ott_uuid_t id;
    ott_uuid_t video_id;
    char file_path[512];
    int width;
    int height;
    time_t created_at;
} thumbnail_t;

// 시청 이력
typedef struct {
    ott_uuid_t id;
    ott_uuid_t user_id;
    ott_uuid_t video_id;
    int last_position_sec;
    bool completed;
    time_t updated_at;
} watch_history_t;

// HTTP Range
typedef struct {
    int64_t start;
    int64_t end;
    bool has_range;
} http_range_t;

#endif // TYPES_H
