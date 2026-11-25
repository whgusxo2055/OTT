#!/bin/sh
set -eu

project_dir=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
output="$project_dir/media/movie-001"
mkdir -p "$output"
ffmpeg -hide_banner -loglevel error -y \
  -f lavfi -i 'testsrc2=size=640x360:rate=30' -t 75 \
  -c:v libx264 -pix_fmt yuv420p -preset veryfast \
  -force_key_frames 'expr:gte(t,n_forced*6)' \
  -f hls -hls_time 6 -hls_playlist_type vod \
  -hls_segment_filename "$output/seg_%05d.ts" "$output/index.m3u8"
