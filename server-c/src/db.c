#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "db.h"
#include "logger.h"
#include "uuid.h"

static db_pool_t db_pool;

int db_init(const char *db_path) {
    int rc = sqlite3_open(db_path, &db_pool.db);
    if (rc != SQLITE_OK) {
        log_error("데이터베이스 열기 실패: %s", sqlite3_errmsg(db_pool.db));
        return -1;
    }

    pthread_mutex_init(&db_pool.mutex, NULL);

    // 외래 키 활성화
    char *err_msg = NULL;
    rc = sqlite3_exec(db_pool.db, "PRAGMA foreign_keys = ON;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        log_error("외래 키 활성화 실패: %s", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    log_info("데이터베이스 초기화 완료: %s", db_path);
    return 0;
}

void db_close(void) {
    if (db_pool.db != NULL) {
        sqlite3_close(db_pool.db);
        pthread_mutex_destroy(&db_pool.mutex);
        log_info("데이터베이스 종료");
    }
}

sqlite3* db_get_connection(void) {
    pthread_mutex_lock(&db_pool.mutex);
    return db_pool.db;
}

void db_release_connection(sqlite3 *db) {
    (void)db; // Unused in single connection model
    pthread_mutex_unlock(&db_pool.mutex);
}

// User operations
int db_create_user(const char *login_id, const char *password_hash, const char *display_name, ott_uuid_t out_id) {
    sqlite3 *db = db_get_connection();
    
    uuid_generate(out_id);
    
    const char *sql = "INSERT INTO users (id, login_id, password_hash, display_name) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("Failed to prepare statement: %s", sqlite3_errmsg(db));
        db_release_connection(db);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, out_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, login_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, display_name, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    db_release_connection(db);
    
    if (rc != SQLITE_DONE) {
        log_error("사용자 생성 실패: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    log_info("사용자 생성 완료: %s (ID: %s)", login_id, out_id);
    return 0;
}

int db_get_user_by_login(const char *login_id, user_t *user) {
    sqlite3 *db = db_get_connection();
    
    const char *sql = "SELECT id, login_id, password_hash, display_name, created_at, updated_at FROM users WHERE login_id = ?";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("Failed to prepare statement: %s", sqlite3_errmsg(db));
        db_release_connection(db);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, login_id, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        strncpy(user->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(user->id) - 1);
        strncpy(user->login_id, (const char*)sqlite3_column_text(stmt, 1), sizeof(user->login_id) - 1);
        strncpy(user->password_hash, (const char*)sqlite3_column_text(stmt, 2), sizeof(user->password_hash) - 1);
        strncpy(user->display_name, (const char*)sqlite3_column_text(stmt, 3), sizeof(user->display_name) - 1);
        
        sqlite3_finalize(stmt);
        db_release_connection(db);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    db_release_connection(db);
    return -1;
}

int db_get_user_by_id(const char *user_id, user_t *user) {
    sqlite3 *db = db_get_connection();
    
    const char *sql = "SELECT id, login_id, password_hash, display_name, created_at, updated_at FROM users WHERE id = ?";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        db_release_connection(db);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        strncpy(user->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(user->id) - 1);
        strncpy(user->login_id, (const char*)sqlite3_column_text(stmt, 1), sizeof(user->login_id) - 1);
        strncpy(user->password_hash, (const char*)sqlite3_column_text(stmt, 2), sizeof(user->password_hash) - 1);
        strncpy(user->display_name, (const char*)sqlite3_column_text(stmt, 3), sizeof(user->display_name) - 1);
        
        sqlite3_finalize(stmt);
        db_release_connection(db);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    db_release_connection(db);
    return -1;
}

// Video operations
int db_create_video(const char *title, const char *description, int duration_sec, ott_uuid_t out_id) {
    sqlite3 *db = db_get_connection();
    
    uuid_generate(out_id);
    
    const char *sql = "INSERT INTO videos (id, title, description, duration_sec) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        db_release_connection(db);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, out_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, duration_sec);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    db_release_connection(db);
    
    if (rc != SQLITE_DONE) {
        log_error("동영상 생성 실패: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    log_info("동영상 생성 완료: %s (ID: %s)", title, out_id);
    return 0;
}

int db_get_video(const char *video_id, video_t *video) {
    sqlite3 *db = db_get_connection();
    
    const char *sql = "SELECT id, title, description, duration_sec, mime_type FROM videos WHERE id = ?";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        db_release_connection(db);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, video_id, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        strncpy(video->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(video->id) - 1);
        strncpy(video->title, (const char*)sqlite3_column_text(stmt, 1), sizeof(video->title) - 1);
        
        const char *desc = (const char*)sqlite3_column_text(stmt, 2);
        if (desc) {
            strncpy(video->description, desc, sizeof(video->description) - 1);
        } else {
            video->description[0] = '\0';
        }
        
        video->duration_sec = sqlite3_column_int(stmt, 3);
        strncpy(video->mime_type, (const char*)sqlite3_column_text(stmt, 4), sizeof(video->mime_type) - 1);
        
        sqlite3_finalize(stmt);
        db_release_connection(db);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    db_release_connection(db);
    return -1;
}

int db_list_videos(int page, int page_size, video_t **videos, int *count, int *total) {
    sqlite3 *db = db_get_connection();
    
    // Get total count
    const char *count_sql = "SELECT COUNT(*) FROM videos";
    sqlite3_stmt *count_stmt;
    sqlite3_prepare_v2(db, count_sql, -1, &count_stmt, NULL);
    if (sqlite3_step(count_stmt) == SQLITE_ROW) {
        *total = sqlite3_column_int(count_stmt, 0);
    }
    sqlite3_finalize(count_stmt);
    
    // Get paginated results
    const char *sql = "SELECT id, title, description, duration_sec, mime_type FROM videos ORDER BY created_at DESC LIMIT ? OFFSET ?";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        db_release_connection(db);
        return -1;
    }
    
    int offset = (page - 1) * page_size;
    sqlite3_bind_int(stmt, 1, page_size);
    sqlite3_bind_int(stmt, 2, offset);
    
    // Count results
    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*count)++;
    }
    sqlite3_reset(stmt);
    
    // Allocate array
    *videos = malloc(sizeof(video_t) * (*count));
    
    // Fill results
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < *count) {
        video_t *v = &(*videos)[i];
        strncpy(v->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(v->id) - 1);
        v->id[sizeof(v->id) - 1] = '\0';
        
        strncpy(v->title, (const char*)sqlite3_column_text(stmt, 1), sizeof(v->title) - 1);
        v->title[sizeof(v->title) - 1] = '\0';
        
        const char *desc = (const char*)sqlite3_column_text(stmt, 2);
        if (desc) {
            strncpy(v->description, desc, sizeof(v->description) - 1);
            v->description[sizeof(v->description) - 1] = '\0';
        } else {
            v->description[0] = '\0';
        }
        
        v->duration_sec = sqlite3_column_int(stmt, 3);
        strncpy(v->mime_type, (const char*)sqlite3_column_text(stmt, 4), sizeof(v->mime_type) - 1);
        v->mime_type[sizeof(v->mime_type) - 1] = '\0';
        i++;
    }
    
    sqlite3_finalize(stmt);
    db_release_connection(db);
    return 0;
}

int db_search_videos(const char *query, int page, int page_size, video_t **videos, int *count, int *total) {
    // For now, simple LIKE search on title
    // Can be enhanced with full-text search later
    sqlite3 *db = db_get_connection();
    
    char search_pattern[512];
    snprintf(search_pattern, sizeof(search_pattern), "%%%s%%", query);
    
    // Get total count
    const char *count_sql = "SELECT COUNT(*) FROM videos WHERE title LIKE ?";
    sqlite3_stmt *count_stmt;
    sqlite3_prepare_v2(db, count_sql, -1, &count_stmt, NULL);
    sqlite3_bind_text(count_stmt, 1, search_pattern, -1, SQLITE_STATIC);
    if (sqlite3_step(count_stmt) == SQLITE_ROW) {
        *total = sqlite3_column_int(count_stmt, 0);
    }
    sqlite3_finalize(count_stmt);
    
    // Get paginated search results
    const char *sql = "SELECT id, title, description, duration_sec, mime_type FROM videos WHERE title LIKE ? ORDER BY created_at DESC LIMIT ? OFFSET ?";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        db_release_connection(db);
        return -1;
    }
    
    int offset = (page - 1) * page_size;
    sqlite3_bind_text(stmt, 1, search_pattern, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, page_size);
    sqlite3_bind_int(stmt, 3, offset);
    
    // Count results
    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*count)++;
    }
    sqlite3_reset(stmt);
    
    // Allocate array
    *videos = malloc(sizeof(video_t) * (*count));
    
    // Fill results
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < *count) {
        video_t *v = &(*videos)[i];
        strncpy(v->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(v->id) - 1);
        v->id[sizeof(v->id) - 1] = '\0';
        
        strncpy(v->title, (const char*)sqlite3_column_text(stmt, 1), sizeof(v->title) - 1);
        v->title[sizeof(v->title) - 1] = '\0';
        
        const char *desc = (const char*)sqlite3_column_text(stmt, 2);
        if (desc) {
            strncpy(v->description, desc, sizeof(v->description) - 1);
            v->description[sizeof(v->description) - 1] = '\0';
        } else {
            v->description[0] = '\0';
        }
        
        v->duration_sec = sqlite3_column_int(stmt, 3);
        strncpy(v->mime_type, (const char*)sqlite3_column_text(stmt, 4), sizeof(v->mime_type) - 1);
        v->mime_type[sizeof(v->mime_type) - 1] = '\0';
        i++;
    }
    
    sqlite3_finalize(stmt);
    db_release_connection(db);
    return 0;
}

// Video file operations
int db_create_video_file(const char *video_id, const char *file_path, int64_t file_size, 
                         int bitrate_kbps, const char *resolution, ott_uuid_t out_id) {
    sqlite3 *db = db_get_connection();
    
    uuid_generate(out_id);
    
    const char *sql = "INSERT INTO video_files (id, video_id, file_path, file_size, bitrate_kbps, resolution) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, out_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, video_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, file_path, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, file_size);
    sqlite3_bind_int(stmt, 5, bitrate_kbps);
    sqlite3_bind_text(stmt, 6, resolution, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    db_release_connection(db);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_video_files(const char *video_id, video_file_t **files, int *count) {
    sqlite3 *db = db_get_connection();
    
    const char *sql = "SELECT id, video_id, file_path, file_size, bitrate_kbps, resolution FROM video_files WHERE video_id = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, video_id, -1, SQLITE_STATIC);
    
    // Count results
    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*count)++;
    }
    sqlite3_reset(stmt);
    
    if (*count == 0) {
        sqlite3_finalize(stmt);
        db_release_connection(db);
        return 0;
    }
    
    *files = malloc(sizeof(video_file_t) * (*count));
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < *count) {
        video_file_t *f = &(*files)[i];
        strncpy(f->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(f->id) - 1);
        strncpy(f->video_id, (const char*)sqlite3_column_text(stmt, 1), sizeof(f->video_id) - 1);
        strncpy(f->file_path, (const char*)sqlite3_column_text(stmt, 2), sizeof(f->file_path) - 1);
        f->file_size = sqlite3_column_int64(stmt, 3);
        f->bitrate_kbps = sqlite3_column_int(stmt, 4);
        
        const char *res = (const char*)sqlite3_column_text(stmt, 5);
        if (res) {
            strncpy(f->resolution, res, sizeof(f->resolution) - 1);
        }
        i++;
    }
    
    sqlite3_finalize(stmt);
    db_release_connection(db);
    return 0;
}

// Thumbnail operations
int db_create_thumbnail(const char *video_id, const char *file_path, int width, int height, ott_uuid_t out_id) {
    sqlite3 *db = db_get_connection();
    
    uuid_generate(out_id);
    
    const char *sql = "INSERT INTO thumbnails (id, video_id, file_path, width, height) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, out_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, video_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, file_path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, width);
    sqlite3_bind_int(stmt, 5, height);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    db_release_connection(db);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_thumbnail(const char *video_id, thumbnail_t *thumbnail) {
    sqlite3 *db = db_get_connection();
    
    const char *sql = "SELECT id, video_id, file_path, width, height FROM thumbnails WHERE video_id = ? LIMIT 1";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, video_id, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        strncpy(thumbnail->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(thumbnail->id) - 1);
        strncpy(thumbnail->video_id, (const char*)sqlite3_column_text(stmt, 1), sizeof(thumbnail->video_id) - 1);
        strncpy(thumbnail->file_path, (const char*)sqlite3_column_text(stmt, 2), sizeof(thumbnail->file_path) - 1);
        thumbnail->width = sqlite3_column_int(stmt, 3);
        thumbnail->height = sqlite3_column_int(stmt, 4);
        
        sqlite3_finalize(stmt);
        db_release_connection(db);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    db_release_connection(db);
    return -1;
}

// Watch history operations
int db_upsert_watch_history(const char *user_id, const char *video_id, int position_sec, bool completed) {
    sqlite3 *db = db_get_connection();
    
    const char *sql = "INSERT INTO watch_history (id, user_id, video_id, last_position_sec, completed) "
                      "VALUES (?, ?, ?, ?, ?) "
                      "ON CONFLICT(user_id, video_id) DO UPDATE SET "
                      "last_position_sec = excluded.last_position_sec, "
                      "completed = excluded.completed, "
                      "updated_at = datetime('now')";
    
    ott_uuid_t id;
    uuid_generate(id);
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, video_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, position_sec);
    sqlite3_bind_int(stmt, 5, completed ? 1 : 0);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    db_release_connection(db);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_watch_history(const char *user_id, const char *video_id, watch_history_t *history) {
    sqlite3 *db = db_get_connection();
    
    const char *sql = "SELECT id, user_id, video_id, last_position_sec, completed FROM watch_history WHERE user_id = ? AND video_id = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, video_id, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        strncpy(history->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(history->id) - 1);
        strncpy(history->user_id, (const char*)sqlite3_column_text(stmt, 1), sizeof(history->user_id) - 1);
        strncpy(history->video_id, (const char*)sqlite3_column_text(stmt, 2), sizeof(history->video_id) - 1);
        history->last_position_sec = sqlite3_column_int(stmt, 3);
        history->completed = sqlite3_column_int(stmt, 4) == 1;
        
        sqlite3_finalize(stmt);
        db_release_connection(db);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    db_release_connection(db);
    return -1;
}
