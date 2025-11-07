#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "civetweb.h"
#include "cJSON.h"
#include "http_handler.h"
#include "auth.h"
#include "db.h"
#include "streaming.h"
#include "json_helper.h"
#include "logger.h"
#include "config.h"

static struct mg_context *ctx = NULL;

// Helper function to get Authorization header
static const char* get_auth_header(struct mg_connection *conn) {
    return mg_get_header(conn, "Authorization");
}

// Helper function to authenticate request
static int authenticate_request(struct mg_connection *conn, user_t *user) {
    const char *auth_header = get_auth_header(conn);
    
    if (auth_header == NULL) {
        cJSON *error = json_create_error("UNAUTHORIZED", "Authorization required");
        json_send_response(conn, 401, error);
        return -1;
    }
    
    if (auth_authenticate_user(auth_header, user) < 0) {
        cJSON *error = json_create_error("UNAUTHORIZED", "Invalid credentials");
        json_send_response(conn, 401, error);
        return -1;
    }
    
    return 0;
}

int handle_auth_check(struct mg_connection *conn, void *cbdata) {
    (void)cbdata;
    
    user_t user;
    if (authenticate_request(conn, &user) < 0) {
        return 1; // Already sent error response
    }
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "ok", 1);
    cJSON_AddItemToObject(response, "user", json_create_user(&user));
    
    json_send_response(conn, 200, response);
    return 1;
}

int handle_videos_list(struct mg_connection *conn, void *cbdata) {
    (void)cbdata;
    
    user_t user;
    if (authenticate_request(conn, &user) < 0) {
        return 1;
    }
    
    // 쿼리 파라미터 파싱
    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *query_string = ri->query_string ? ri->query_string : "";
    size_t query_string_len = strlen(query_string);
    
    char query_buf[256] = "";
    char page_buf[16] = "1";
    char page_size_buf[16] = "20";
    
    mg_get_var(query_string, query_string_len, "query", query_buf, sizeof(query_buf));
    mg_get_var(query_string, query_string_len, "page", page_buf, sizeof(page_buf));
    mg_get_var(query_string, query_string_len, "pageSize", page_size_buf, sizeof(page_size_buf));
    
    int page = atoi(page_buf);
    int page_size = atoi(page_size_buf);
    
    if (page < 1) page = 1;
    if (page_size < 1 || page_size > 100) page_size = 20;
    
    video_t *videos = NULL;
    int count = 0;
    int total = 0;
    
    if (strlen(query_buf) > 0) {
        db_search_videos(query_buf, page, page_size, &videos, &count, &total);
    } else {
        db_list_videos(page, page_size, &videos, &count, &total);
    }
    
    cJSON *response = json_create_video_list(videos, count, page, page_size, total);
    json_send_response(conn, 200, response);
    
    if (videos != NULL) {
        free(videos);
    }
    
    return 1;
}

int handle_video_detail(struct mg_connection *conn, void *cbdata) {
    (void)cbdata;
    
    user_t user;
    if (authenticate_request(conn, &user) < 0) {
        return 1;
    }
    
    // Extract video ID from path: /api/videos/:id
    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *uri = ri->local_uri;
    
    // Find the ID part (after /api/videos/)
    const char *id_start = strstr(uri, "/api/videos/");
    if (id_start == NULL) {
        cJSON *error = json_create_error("BAD_REQUEST", "Invalid URI");
        json_send_response(conn, 400, error);
        return 1;
    }
    
    id_start += strlen("/api/videos/");
    char video_id[64];
    strncpy(video_id, id_start, sizeof(video_id) - 1);
    video_id[sizeof(video_id) - 1] = '\0';
    
    // Remove any trailing path
    char *slash = strchr(video_id, '/');
    if (slash != NULL) {
        *slash = '\0';
    }
    
    video_t video;
    if (db_get_video(video_id, &video) < 0) {
        cJSON *error = json_create_error("NOT_FOUND", "Video not found");
        json_send_response(conn, 404, error);
        return 1;
    }
    
    char thumbnail_url[256];
    snprintf(thumbnail_url, sizeof(thumbnail_url), "/api/videos/%s/thumbnail", video_id);
    
    cJSON *response = json_create_video(&video, thumbnail_url);
    
    // Add files array
    video_file_t *files = NULL;
    int file_count = 0;
    db_get_video_files(video_id, &files, &file_count);
    
    cJSON *files_array = cJSON_CreateArray();
    for (int i = 0; i < file_count; i++) {
        cJSON *file_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(file_obj, "path", files[i].file_path);
        cJSON_AddNumberToObject(file_obj, "size", files[i].file_size);
        cJSON_AddNumberToObject(file_obj, "bitrate", files[i].bitrate_kbps);
        cJSON_AddItemToArray(files_array, file_obj);
    }
    cJSON_AddItemToObject(response, "files", files_array);
    
    if (files != NULL) {
        free(files);
    }
    
    json_send_response(conn, 200, response);
    return 1;
}

int handle_video_thumbnail(struct mg_connection *conn, void *cbdata) {
    (void)cbdata;
    
    // Extract video ID
    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *uri = ri->local_uri;
    
    const char *id_start = strstr(uri, "/api/videos/");
    if (id_start == NULL) {
        mg_send_http_error(conn, 400, "Invalid URI");
        return 1;
    }
    
    id_start += strlen("/api/videos/");
    char video_id[64];
    strncpy(video_id, id_start, sizeof(video_id) - 1);
    video_id[sizeof(video_id) - 1] = '\0';
    
    char *slash = strchr(video_id, '/');
    if (slash != NULL) {
        *slash = '\0';
    }
    
    thumbnail_t thumbnail;
    if (db_get_thumbnail(video_id, &thumbnail) < 0) {
        mg_send_http_error(conn, 404, "Thumbnail not found");
        return 1;
    }
    
    // Send thumbnail file
    mg_send_file(conn, thumbnail.file_path);
    return 1;
}

int handle_video_stream(struct mg_connection *conn, void *cbdata) {
    (void)cbdata;
    
    user_t user;
    if (authenticate_request(conn, &user) < 0) {
        return 1;
    }
    
    // Extract video ID
    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *uri = ri->local_uri;
    
    const char *id_start = strstr(uri, "/api/videos/");
    if (id_start == NULL) {
        mg_send_http_error(conn, 400, "Invalid URI");
        return 1;
    }
    
    id_start += strlen("/api/videos/");
    char video_id[64];
    strncpy(video_id, id_start, sizeof(video_id) - 1);
    video_id[sizeof(video_id) - 1] = '\0';
    
    char *slash = strchr(video_id, '/');
    if (slash != NULL) {
        *slash = '\0';
    }
    
    // Get video files
    video_file_t *files = NULL;
    int file_count = 0;
    db_get_video_files(video_id, &files, &file_count);
    
    if (file_count == 0) {
        mg_send_http_error(conn, 404, "Video file not found");
        if (files != NULL) free(files);
        return 1;
    }
    
    // Use first file (can be enhanced for multi-bitrate selection)
    const char *file_path = files[0].file_path;
    int bitrate = files[0].bitrate_kbps;
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) != 0) {
        mg_send_http_error(conn, 404, "File not found");
        free(files);
        return 1;
    }
    
    // Parse Range header
    const char *range_header = mg_get_header(conn, "Range");
    http_range_t range;
    
    if (streaming_parse_range(range_header, st.st_size, &range) < 0) {
        mg_send_http_error(conn, 416, "Range not satisfiable");
        free(files);
        return 1;
    }
    
    // 시작 위치 파라미터 확인
    const char *query_string = ri->query_string ? ri->query_string : "";
    size_t query_string_len = strlen(query_string);
    char start_param[16];
    if (mg_get_var(query_string, query_string_len, "start", start_param, sizeof(start_param)) > 0) {
        int64_t offset;
        if (streaming_parse_start_param(start_param, &offset, bitrate) == 0) {
            range.start = offset;
            range.end = st.st_size - 1;
            range.has_range = true;
        }
    }
    
    // 파일 스트리밍
    video_t video;
    db_get_video(video_id, &video);
    streaming_send_video(conn, file_path, &range, video.mime_type);
    
    free(files);
    return 1;
}

int handle_watch_history_get(struct mg_connection *conn, void *cbdata) {
    (void)cbdata;
    
    user_t user;
    if (authenticate_request(conn, &user) < 0) {
        return 1;
    }
    
    // videoId 파라미터 가져오기
    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *query_string = ri->query_string ? ri->query_string : "";
    size_t query_string_len = strlen(query_string);
    char video_id[64];
    
    if (mg_get_var(query_string, query_string_len, "videoId", video_id, sizeof(video_id)) <= 0) {
        cJSON *error = json_create_error("BAD_REQUEST", "videoId parameter required");
        json_send_response(conn, 400, error);
        return 1;
    }
    
    watch_history_t history;
    if (db_get_watch_history(user.id, video_id, &history) < 0) {
        // No history found - return 204
        mg_printf(conn, "HTTP/1.1 204 No Content\r\n");
        mg_printf(conn, "Content-Length: 0\r\n");
        mg_printf(conn, "\r\n");
        return 1;
    }
    
    cJSON *response = json_create_watch_history(&history);
    json_send_response(conn, 200, response);
    return 1;
}

int handle_watch_progress_post(struct mg_connection *conn, void *cbdata) {
    (void)cbdata;
    
    user_t user;
    if (authenticate_request(conn, &user) < 0) {
        return 1;
    }
    
    // Extract video ID from URI
    const struct mg_request_info *ri = mg_get_request_info(conn);
    const char *uri = ri->local_uri;
    
    const char *id_start = strstr(uri, "/api/videos/");
    if (id_start == NULL) {
        mg_send_http_error(conn, 400, "Invalid URI");
        return 1;
    }
    
    id_start += strlen("/api/videos/");
    char video_id[64];
    strncpy(video_id, id_start, sizeof(video_id) - 1);
    video_id[sizeof(video_id) - 1] = '\0';
    
    char *slash = strchr(video_id, '/');
    if (slash != NULL) {
        *slash = '\0';
    }
    
    // Read request body
    char body[1024];
    int body_len = mg_read(conn, body, sizeof(body) - 1);
    if (body_len <= 0) {
        mg_send_http_error(conn, 400, "Empty request body");
        return 1;
    }
    body[body_len] = '\0';
    
    // Parse JSON
    cJSON *json = cJSON_Parse(body);
    if (json == NULL) {
        mg_send_http_error(conn, 400, "Invalid JSON");
        return 1;
    }
    
    cJSON *position_json = cJSON_GetObjectItem(json, "positionSec");
    cJSON *completed_json = cJSON_GetObjectItem(json, "completed");
    
    if (position_json == NULL || !cJSON_IsNumber(position_json)) {
        cJSON_Delete(json);
        mg_send_http_error(conn, 400, "positionSec required");
        return 1;
    }
    
    int position_sec = (int)cJSON_GetNumberValue(position_json);
    bool completed = false;
    
    if (completed_json != NULL && cJSON_IsBool(completed_json)) {
        completed = cJSON_IsTrue(completed_json);
    }
    
    cJSON_Delete(json);
    
    // Update watch history
    if (db_upsert_watch_history(user.id, video_id, position_sec, completed) < 0) {
        mg_send_http_error(conn, 500, "Failed to update watch history");
        return 1;
    }
    
    // Return 204 No Content
    mg_printf(conn, "HTTP/1.1 204 No Content\r\n");
    mg_printf(conn, "Content-Length: 0\r\n");
    mg_printf(conn, "\r\n");
    
    return 1;
}

int http_server_init(void) {
    mg_init_library(0);
    return 0;
}

int http_server_start(void) {
    const char *options[] = {
        "listening_ports", SERVER_PORT,
        "num_threads", "4",
        "document_root", WEB_DIR,
        NULL
    };
    
    ctx = mg_start(NULL, NULL, options);
    if (ctx == NULL) {
        log_error("HTTP 서버 시작 실패");
        return -1;
    }
    
    // API 핸들러 등록
    mg_set_request_handler(ctx, "/api/auth/check", handle_auth_check, NULL);
    mg_set_request_handler(ctx, "/api/videos$", handle_videos_list, NULL);
    mg_set_request_handler(ctx, "/api/videos/*/stream", handle_video_stream, NULL);
    mg_set_request_handler(ctx, "/api/videos/*/thumbnail", handle_video_thumbnail, NULL);
    mg_set_request_handler(ctx, "/api/videos/*/progress", handle_watch_progress_post, NULL);
    mg_set_request_handler(ctx, "/api/videos/*", handle_video_detail, NULL);
    mg_set_request_handler(ctx, "/api/users/me/history", handle_watch_history_get, NULL);
    
    log_info("HTTP 서버가 포트 %s에서 시작되었습니다", SERVER_PORT);
    return 0;
}

void http_server_stop(void) {
    if (ctx != NULL) {
        mg_stop(ctx);
        mg_exit_library();
        log_info("HTTP 서버 중지");
    }
}
