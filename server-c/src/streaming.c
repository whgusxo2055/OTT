#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "civetweb.h"
#include "streaming.h"
#include "logger.h"
#include "config.h"

int streaming_parse_range(const char *range_header, int64_t file_size, http_range_t *range) {
    if (range_header == NULL || range == NULL) {
        range->has_range = false;
        return 0;
    }

    // "bytes=START-END" 형식 파싱
    if (strncmp(range_header, "bytes=", 6) != 0) {
        log_warn("잘못된 range 헤더 형식: %s", range_header);
        range->has_range = false;
        return -1;
    }

    const char *range_spec = range_header + 6;
    char *dash = strchr(range_spec, '-');
    
    if (dash == NULL) {
        log_warn("Invalid range specification: %s", range_spec);
        range->has_range = false;
        return -1;
    }

    // Parse start
    if (dash == range_spec) {
        // Suffix range: "-500" means last 500 bytes
        range->end = file_size - 1;
        range->start = file_size - atoll(dash + 1);
        if (range->start < 0) range->start = 0;
    } else {
        range->start = atoll(range_spec);
        
        if (*(dash + 1) == '\0') {
            // Open-ended: "1000-" means from 1000 to end
            range->end = file_size - 1;
        } else {
            // Closed range: "1000-2000"
            range->end = atoll(dash + 1);
        }
    }

    // Validate range
    if (range->start < 0 || range->start >= file_size) {
        log_warn("Range start out of bounds: %lld (file size: %lld)", range->start, file_size);
        return -1;
    }

    if (range->end >= file_size) {
        range->end = file_size - 1;
    }

    if (range->start > range->end) {
        log_warn("Invalid range: start > end (%lld > %lld)", range->start, range->end);
        return -1;
    }

    range->has_range = true;
    log_debug("Parsed range: %lld-%lld (size: %lld)", range->start, range->end, range->end - range->start + 1);
    
    return 0;
}

int streaming_send_video(struct mg_connection *conn, const char *file_path, 
                         const http_range_t *range, const char *mime_type) {
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        log_error("Failed to open file: %s", file_path);
        mg_send_http_error(conn, 404, "File not found");
        return -1;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    int64_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    int64_t start = 0;
    int64_t end = file_size - 1;
    int64_t content_length = file_size;

    if (range != NULL && range->has_range) {
        start = range->start;
        end = range->end;
        content_length = end - start + 1;

        // Send 206 Partial Content
        mg_printf(conn, "HTTP/1.1 206 Partial Content\r\n");
        mg_printf(conn, "Content-Type: %s\r\n", mime_type);
        mg_printf(conn, "Content-Length: %lld\r\n", content_length);
        mg_printf(conn, "Content-Range: bytes %lld-%lld/%lld\r\n", start, end, file_size);
        mg_printf(conn, "Accept-Ranges: bytes\r\n");
        mg_printf(conn, "Connection: keep-alive\r\n");
        mg_printf(conn, "\r\n");

        log_info("동영상 스트리밍: %s (범위: %lld-%lld/%lld)", file_path, start, end, file_size);
    } else {
        // 전체 컨텐츠를 200 OK로 전송
        mg_printf(conn, "HTTP/1.1 200 OK\r\n");
        mg_printf(conn, "Content-Type: %s\r\n", mime_type);
        mg_printf(conn, "Content-Length: %lld\r\n", content_length);
        mg_printf(conn, "Accept-Ranges: bytes\r\n");
        mg_printf(conn, "Connection: keep-alive\r\n");
        mg_printf(conn, "\r\n");

        log_info("동영상 스트리밍: %s (전체 컨텐츠: %lld 바이트)", file_path, file_size);
    }

    // Seek to start position
    if (fseek(fp, start, SEEK_SET) != 0) {
        log_error("Failed to seek to position %lld", start);
        fclose(fp);
        return -1;
    }

    // Stream file in chunks
    char buffer[CHUNK_SIZE];
    int64_t bytes_sent = 0;
    int64_t bytes_to_send = content_length;

    while (bytes_sent < bytes_to_send) {
        size_t chunk_size = CHUNK_SIZE;
        if (bytes_sent + chunk_size > bytes_to_send) {
            chunk_size = bytes_to_send - bytes_sent;
        }

        size_t bytes_read = fread(buffer, 1, chunk_size, fp);
        if (bytes_read == 0) {
            if (feof(fp)) {
                log_debug("End of file reached");
                break;
            }
            if (ferror(fp)) {
                log_error("Error reading file");
                break;
            }
        }

        int bytes_written = mg_write(conn, buffer, bytes_read);
        if (bytes_written <= 0) {
            log_warn("Client disconnected during streaming");
            break;
        }

        bytes_sent += bytes_read;
    }

    fclose(fp);
    log_debug("Streamed %lld/%lld bytes", bytes_sent, bytes_to_send);
    
    return 0;
}

int streaming_parse_start_param(const char *start_param, int64_t *offset_bytes, int bitrate_kbps) {
    if (start_param == NULL || offset_bytes == NULL) {
        return -1;
    }

    // Parse start time in seconds
    int start_sec = atoi(start_param);
    if (start_sec < 0) {
        return -1;
    }

    // Estimate byte offset based on bitrate
    // This is approximate; actual offset depends on video encoding
    if (bitrate_kbps > 0) {
        *offset_bytes = (int64_t)start_sec * bitrate_kbps * 125; // 1 kbps = 125 bytes/sec
    } else {
        // Fallback: assume average bitrate of 2 Mbps
        *offset_bytes = (int64_t)start_sec * 2000 * 125;
    }

    log_debug("Start parameter: %d sec -> estimated offset: %lld bytes", start_sec, *offset_bytes);
    return 0;
}
