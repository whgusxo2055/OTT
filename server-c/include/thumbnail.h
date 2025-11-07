#ifndef THUMBNAIL_H
#define THUMBNAIL_H

#include "types.h"

// Generate thumbnail for a video file
int thumbnail_generate(const char *video_path, const char *output_path, 
                      int width, double offset_sec);

// Get video duration using ffprobe
int thumbnail_get_duration(const char *video_path, double *duration_sec);

// Generate thumbnail for video and save to database
int thumbnail_generate_and_save(const char *video_id, const char *video_path);

#endif // THUMBNAIL_H
