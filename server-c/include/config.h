#ifndef CONFIG_H
#define CONFIG_H

#define SERVER_PORT "8080"
#define SERVER_THREADS 4
#define DB_PATH "app.db"
#define MEDIA_DIR "./media"
#define VIDEO_DIR "./media/videos"
#define THUMBNAIL_DIR "./media/thumbnails"
#define WEB_DIR "./web"

#define MAX_PATH_LEN 1024
#define MAX_QUERY_LEN 2048
#define CHUNK_SIZE (64 * 1024)  // 64KB chunks for streaming

#endif // CONFIG_H
