#!/bin/bash

# Add video to OTT Streaming Server

if [ $# -lt 3 ]; then
    echo "사용법: $0 <video_file> <title> <description>"
    echo "예제: $0 /path/to/video.mp4 '영화 제목' '영화 설명'"
    exit 1
fi

VIDEO_FILE="$1"
TITLE="$2"
DESCRIPTION="$3"

if [ ! -f "$VIDEO_FILE" ]; then
    echo "❌ 비디오 파일을 찾을 수 없습니다: $VIDEO_FILE"
    exit 1
fi

# Get project root
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT" || exit

# Generate UUID for video
VIDEO_UUID=$(uuidgen | tr '[:upper:]' '[:lower:]')
FILE_UUID=$(uuidgen | tr '[:upper:]' '[:lower:]')

# Get video duration using ffprobe
echo "📹 비디오 정보 추출 중..."
DURATION=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$VIDEO_FILE" 2>/dev/null)
DURATION_SEC=$(printf "%.0f" "$DURATION")

# Get file size
FILE_SIZE=$(stat -f%z "$VIDEO_FILE" 2>/dev/null || stat -c%s "$VIDEO_FILE" 2>/dev/null)

# Copy video file to media directory
VIDEO_FILENAME="${VIDEO_UUID}.mp4"
VIDEO_PATH="./media/videos/${VIDEO_FILENAME}"

echo "📁 비디오 파일 복사 중..."
cp "$VIDEO_FILE" "$VIDEO_PATH"

if [ $? -ne 0 ]; then
    echo "❌ 비디오 파일 복사 실패"
    exit 1
fi

# Insert video into database
echo "💾 데이터베이스에 비디오 정보 저장 중..."
cd server-c || exit
sqlite3 app.db <<EOF
INSERT INTO videos (id, title, description, duration_sec, mime_type)
VALUES ('$VIDEO_UUID', '$TITLE', '$DESCRIPTION', $DURATION_SEC, 'video/mp4');

INSERT INTO video_files (id, video_id, file_path, file_size, bitrate_kbps, resolution)
VALUES ('$FILE_UUID', '$VIDEO_UUID', '$VIDEO_PATH', $FILE_SIZE, 2000, '1920x1080');
EOF

if [ $? -ne 0 ]; then
    echo "❌ 데이터베이스 저장 실패"
    rm -f "$VIDEO_PATH"
    exit 1
fi

# Generate thumbnail
echo "🖼️  썸네일 생성 중..."
THUMB_PATH="../media/thumbnails/${VIDEO_UUID}.jpg"
ffmpeg -ss 5 -i "$VIDEO_PATH" -vframes 1 -vf scale=320:-1 "$THUMB_PATH" -y 2>&1 > /dev/null

if [ -f "$THUMB_PATH" ]; then
    THUMB_UUID=$(uuidgen | tr '[:upper:]' '[:lower:]')
    sqlite3 app.db <<EOF
INSERT INTO thumbnails (id, video_id, file_path, width, height)
VALUES ('$THUMB_UUID', '$VIDEO_UUID', '$THUMB_PATH', 320, 180);
EOF
    echo "✅ 썸네일 생성 완료"
else
    echo "⚠️  썸네일 생성 실패 (계속 진행)"
fi

echo ""
echo "✅ 비디오 추가 완료!"
echo "   ID: $VIDEO_UUID"
echo "   제목: $TITLE"
echo "   길이: ${DURATION_SEC}초"
echo "   파일: $VIDEO_PATH"
