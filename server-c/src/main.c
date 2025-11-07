#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sodium.h>
#include "config.h"
#include "logger.h"
#include "db.h"
#include "http_handler.h"
#include "thread_pool.h"

static volatile int keep_running = 1;

void signal_handler(int signum) {
    (void)signum;
    log_info("종료 시그널을 받았습니다");
    keep_running = 0;
}

void print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║   OTT 동영상 스트리밍 서버 v1.0          ║\n");
    printf("║   HTTP Range 206 | 멀티스레드            ║\n");
    printf("╚═══════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    print_banner();
    
    // 로거 초기화
    logger_init(LOG_INFO);
    log_info("OTT 스트리밍 서버를 시작합니다...");
    
    // libsodium 초기화
    if (sodium_init() < 0) {
        log_error("libsodium 초기화 실패");
        return 1;
    }
    log_info("libsodium 초기화 완료");
    
    // 데이터베이스 초기화
    if (db_init(DB_PATH) < 0) {
        log_error("데이터베이스 초기화 실패");
        return 1;
    }
    
    // HTTP 서버 초기화
    if (http_server_init() < 0) {
        log_error("HTTP 서버 초기화 실패");
        db_close();
        return 1;
    }
    
    // HTTP 서버 시작
    if (http_server_start() < 0) {
        log_error("HTTP 서버 시작 실패");
        db_close();
        return 1;
    }
    
    // 시그널 핸들러 설정
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    log_info("서버가 준비되었습니다!");
    log_info("서버 주소: http://localhost:%s", SERVER_PORT);
    log_info("종료하려면 Ctrl+C를 누르세요");
    
    // 메인 루프
    while (keep_running) {
        sleep(1);
    }
    
    // 정리
    log_info("서버를 종료합니다...");
    http_server_stop();
    db_close();
    
    log_info("서버가 정상적으로 종료되었습니다");
    return 0;
}
