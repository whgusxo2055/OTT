#ifndef STREAMING_H
#define STREAMING_H

#include <stdbool.h>
#include <stdint.h>
#include "types.h"

// Parse HTTP Range header
int streaming_parse_range(const char *range_header, int64_t file_size, http_range_t *range);

// Stream video file with range support
int streaming_send_video(struct mg_connection *conn, const char *file_path, 
                         const http_range_t *range, const char *mime_type);

// Calculate start position from query parameter (e.g., ?start=630)
int streaming_parse_start_param(const char *start_param, int64_t *offset_bytes, 
                                int bitrate_kbps);

#endif // STREAMING_H
