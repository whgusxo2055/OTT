#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "civetweb.h"

// Initialize HTTP server
int http_server_init(void);

// Start HTTP server
int http_server_start(void);

// Stop HTTP server
void http_server_stop(void);

// API Handlers
int handle_auth_check(struct mg_connection *conn, void *cbdata);
int handle_videos_list(struct mg_connection *conn, void *cbdata);
int handle_video_detail(struct mg_connection *conn, void *cbdata);
int handle_video_thumbnail(struct mg_connection *conn, void *cbdata);
int handle_video_stream(struct mg_connection *conn, void *cbdata);
int handle_watch_history_get(struct mg_connection *conn, void *cbdata);
int handle_watch_progress_post(struct mg_connection *conn, void *cbdata);

#endif // HTTP_HANDLER_H
