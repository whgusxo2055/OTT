#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "db.h"
#include "uuid.h"
#include "thumbnail.h"
#include "logger.h"
#include "config.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "사용법: %s <video_path> <title> <description> [duration_sec]\n", argv[0]);
        return 1;
    }
    
    const char *source_path = argv[1];
    const char *title = argv[2];
    const char *description = argv[3];
    int duration_sec = (argc > 4) ? atoi(argv[4]) : 0;
    
    // 파일 존재 확인
    struct stat st;
    if (stat(source_path, &st) != 0) {
        fprintf(stderr, "❌ 파일을 찾을 수 없습니다: %s\n", source_path);
        return 1;
    }
    
    // 로거 초기화
    logger_init(LOG_INFO);
    
    // 데이터베이스 초기화
    if (db_init("app.db") < 0) {
        fprintf(stderr, "❌ 데이터베이스 초기화 실패\n");
        return 1;
    }
    
    // Duration 가져오기 (제공되지 않은 경우)
    if (duration_sec == 0) {
        double duration_double;
        if (thumbnail_get_duration(source_path, &duration_double) == 0) {
            duration_sec = (int)duration_double;
        }
    }
    
    // 비디오 생성
    ott_uuid_t video_id;
    if (db_create_video(title, description, duration_sec, video_id) < 0) {
        fprintf(stderr, "❌ 비디오 생성 실패\n");
        db_close();
        return 1;
    }
    
    printf("✅ 비디오 생성 완료: %s\n", video_id);
    
    // 비디오 파일 복사
    char video_filename[128];
    snprintf(video_filename, sizeof(video_filename), "%s.mp4", video_id);
    
    char dest_path[1024];
    snprintf(dest_path, sizeof(dest_path), "../media/videos/%s", video_filename);
    
    // 파일 복사 (간단한 cp 명령 사용)
    char copy_cmd[2048];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" \"%s\"", source_path, dest_path);
    if (system(copy_cmd) != 0) {
        fprintf(stderr, "❌ 파일 복사 실패\n");
        db_close();
        return 1;
    }
    
    printf("✅ 파일 복사 완료: %s\n", dest_path);
    
    // 비디오 파일 정보 DB에 추가
    ott_uuid_t file_id;
    if (db_create_video_file(video_id, dest_path, st.st_size, 2000, "1920x1080", file_id) < 0) {
        fprintf(stderr, "❌ 비디오 파일 정보 저장 실패\n");
        db_close();
        return 1;
    }
    
    printf("✅ 비디오 파일 정보 저장 완료\n");
    
    // 썸네일 자동 생성
    printf("🖼️  썸네일 생성 중...\n");
    if (thumbnail_generate_and_save(video_id, dest_path) == 0) {
        printf("✅ 썸네일 생성 완료\n");
    } else {
        fprintf(stderr, "⚠️  썸네일 생성 실패 (계속 진행)\n");
    }
    
    // 완료
    printf("\n✅ 비디오 추가 완료!\n");
    printf("   ID: %s\n", video_id);
    printf("   제목: %s\n", title);
    printf("   길이: %d초\n", duration_sec);
    
    db_close();
    return 0;
}
