#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include "cJSON.h"
#include "types.h"

// Create JSON response for video
cJSON* json_create_video(const video_t *video, const char *thumbnail_url);

// Create JSON response for video list
cJSON* json_create_video_list(video_t *videos, int count, int page, int page_size, int total);

// Create JSON response for user
cJSON* json_create_user(const user_t *user);

// Create JSON response for watch history
cJSON* json_create_watch_history(const watch_history_t *history);

// Create JSON error response
cJSON* json_create_error(const char *code, const char *message);

// Send JSON response via CivetWeb
void json_send_response(struct mg_connection *conn, int status_code, cJSON *json);

#endif // JSON_HELPER_H
