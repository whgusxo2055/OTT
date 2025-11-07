#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <stdbool.h>
#include <pthread.h>
#include "types.h"

// 데이터베이스 연결 풀 (스레드 안전성)
typedef struct {
    sqlite3 *db;
    pthread_mutex_t mutex;
} db_pool_t;

// 데이터베이스 초기화
int db_init(const char *db_path);

// 데이터베이스 종료
void db_close(void);

// 데이터베이스 연결 가져오기 (스레드 안전)
sqlite3* db_get_connection(void);

// 데이터베이스 연결 반환
void db_release_connection(sqlite3 *db);

// 사용자 관련 작업
int db_create_user(const char *login_id, const char *password_hash, const char *display_name, ott_uuid_t out_id);
int db_get_user_by_login(const char *login_id, user_t *user);
int db_get_user_by_id(const char *user_id, user_t *user);

// 동영상 관련 작업
int db_create_video(const char *title, const char *description, int duration_sec, ott_uuid_t out_id);
int db_get_video(const char *video_id, video_t *video);
int db_list_videos(int page, int page_size, video_t **videos, int *count, int *total);
int db_search_videos(const char *query, int page, int page_size, video_t **videos, int *count, int *total);

// 동영상 파일 관련 작업
int db_create_video_file(const char *video_id, const char *file_path, int64_t file_size, 
                         int bitrate_kbps, const char *resolution, ott_uuid_t out_id);
int db_get_video_files(const char *video_id, video_file_t **files, int *count);

// 썸네일 관련 작업
int db_create_thumbnail(const char *video_id, const char *file_path, int width, int height, ott_uuid_t out_id);
int db_get_thumbnail(const char *video_id, thumbnail_t *thumbnail);

// 시청 이력 관련 작업
int db_upsert_watch_history(const char *user_id, const char *video_id, int position_sec, bool completed);
int db_get_watch_history(const char *user_id, const char *video_id, watch_history_t *history);

#endif // DB_H
