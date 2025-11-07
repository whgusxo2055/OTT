#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "civetweb.h"
#include "json_helper.h"
#include "logger.h"

cJSON* json_create_video(const video_t *video, const char *thumbnail_url) {
    cJSON *json = cJSON_CreateObject();
    
    cJSON_AddStringToObject(json, "id", video->id);
    cJSON_AddStringToObject(json, "title", video->title);
    cJSON_AddStringToObject(json, "description", video->description);
    cJSON_AddNumberToObject(json, "durationSec", video->duration_sec);
    cJSON_AddStringToObject(json, "mimeType", video->mime_type);
    
    if (thumbnail_url != NULL) {
        cJSON_AddStringToObject(json, "thumbnailUrl", thumbnail_url);
    }
    
    return json;
}

cJSON* json_create_video_list(video_t *videos, int count, int page, int page_size, int total) {
    cJSON *json = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();
    
    for (int i = 0; i < count; i++) {
        char thumbnail_url[256];
        snprintf(thumbnail_url, sizeof(thumbnail_url), "/api/videos/%s/thumbnail", videos[i].id);
        
        cJSON *video_json = json_create_video(&videos[i], thumbnail_url);
        cJSON_AddItemToArray(items, video_json);
    }
    
    cJSON_AddItemToObject(json, "items", items);
    cJSON_AddNumberToObject(json, "page", page);
    cJSON_AddNumberToObject(json, "pageSize", page_size);
    cJSON_AddNumberToObject(json, "total", total);
    
    return json;
}

cJSON* json_create_user(const user_t *user) {
    cJSON *json = cJSON_CreateObject();
    
    cJSON_AddStringToObject(json, "id", user->id);
    cJSON_AddStringToObject(json, "loginId", user->login_id);
    cJSON_AddStringToObject(json, "displayName", user->display_name);
    
    return json;
}

cJSON* json_create_watch_history(const watch_history_t *history) {
    cJSON *json = cJSON_CreateObject();
    
    cJSON_AddStringToObject(json, "videoId", history->video_id);
    cJSON_AddNumberToObject(json, "lastPositionSec", history->last_position_sec);
    cJSON_AddBoolToObject(json, "completed", history->completed);
    
    return json;
}

cJSON* json_create_error(const char *code, const char *message) {
    cJSON *json = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();
    
    cJSON_AddStringToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(json, "error", error);
    
    return json;
}

void json_send_response(struct mg_connection *conn, int status_code, cJSON *json) {
    char *json_str = cJSON_PrintUnformatted(json);
    
    mg_printf(conn, "HTTP/1.1 %d %s\r\n", status_code, 
              status_code == 200 ? "OK" : 
              status_code == 204 ? "No Content" :
              status_code == 401 ? "Unauthorized" :
              status_code == 404 ? "Not Found" : "Error");
    mg_printf(conn, "Content-Type: application/json\r\n");
    mg_printf(conn, "Content-Length: %lu\r\n", strlen(json_str));
    mg_printf(conn, "Connection: close\r\n");
    mg_printf(conn, "\r\n");
    mg_printf(conn, "%s", json_str);
    
    cJSON_Delete(json);
    free(json_str);
}
