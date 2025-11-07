#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "thumbnail.h"
#include "logger.h"
#include "db.h"
#include "uuid.h"
#include "config.h"

int thumbnail_get_duration(const char *video_path, double *duration_sec) {
    if (video_path == NULL || duration_sec == NULL) {
        return -1;
    }

    char command[2048];
    snprintf(command, sizeof(command),
             "ffprobe -v error -show_entries format=duration "
             "-of default=noprint_wrappers=1:nokey=1 \"%s\"",
             video_path);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        log_error("ffprobe 실행 실패");
        return -1;
    }

    char duration_str[64];
    if (fgets(duration_str, sizeof(duration_str), fp) == NULL) {
        pclose(fp);
        log_error("ffprobe에서 길이 읽기 실패");
        return -1;
    }

    pclose(fp);
    *duration_sec = atof(duration_str);
    
    log_debug("동영상 길이: %.2f초", *duration_sec);
    return 0;
}

int thumbnail_generate(const char *video_path, const char *output_path, 
                      int width, double offset_sec) {
    if (video_path == NULL || output_path == NULL) {
        return -1;
    }

    // Create output directory if it doesn't exist
    char dir_path[512];
    strncpy(dir_path, output_path, sizeof(dir_path) - 1);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
        mkdir(dir_path, 0755);
    }

    // Build ffmpeg command
    char command[2048];
    snprintf(command, sizeof(command),
             "ffmpeg -ss %.2f -i \"%s\" -vframes 1 -vf scale=%d:-1 \"%s\" -y 2>&1",
             offset_sec, video_path, width, output_path);

    log_debug("썸네일 생성: %s", command);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        log_error("ffmpeg 실행 실패");
        return -1;
    }

    // Read ffmpeg output (for debugging)
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Optionally log ffmpeg output
        // log_debug("ffmpeg: %s", line);
    }

    int status = pclose(fp);
    if (status != 0) {
        log_error("ffmpeg가 상태 코드 %d로 실패했습니다", status);
        return -1;
    }

    // Verify output file exists
    struct stat st;
    if (stat(output_path, &st) != 0) {
        log_error("썸네일 파일이 생성되지 않았습니다: %s", output_path);
        return -1;
    }

    log_info("썸네일 생성 완료: %s (크기: %lld 바이트)", output_path, (long long)st.st_size);
    return 0;
}

int thumbnail_generate_and_save(const char *video_id, const char *video_path) {
    if (video_id == NULL || video_path == NULL) {
        return -1;
    }

    // Get video duration
    double duration_sec;
    if (thumbnail_get_duration(video_path, &duration_sec) < 0) {
        log_warn("동영상 길이를 가져오지 못했습니다. 고정 오프셋 사용");
        duration_sec = 100.0; // Fallback
    }

    // Calculate offset (10% of duration or 5 seconds, whichever is smaller)
    double offset_sec = duration_sec * 0.1;
    if (offset_sec > 5.0) {
        offset_sec = 5.0;
    }

    // Generate output path
    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "%s/%s.jpg", THUMBNAIL_DIR, video_id);

    // Generate thumbnail
    if (thumbnail_generate(video_path, output_path, 320, offset_sec) < 0) {
        return -1;
    }

    // Save to database
    ott_uuid_t thumb_id;
    if (db_create_thumbnail(video_id, output_path, 320, 180, thumb_id) < 0) {
        log_error("데이터베이스에 썸네일 저장 실패");
        return -1;
    }

    log_info("동영상 %s의 썸네일 저장 완료: %s", video_id, output_path);
    return 0;
}
