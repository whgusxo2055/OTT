# OTT 동영상 스트리밍 서버 프로젝트 명세서

작성일: 2025-11-07  
제출기한: 2025-12-10

---

## 1) 복사해서 바로 쓰는 마스터 프롬프트

아래 프롬프트를 개발팀/에이전트에게 전달하면, 요구사항에 맞춘 OTT 스트리밍 서버를 end-to-end로 구현하도록 유도할 수 있습니다.

— 복사 시작 —
당신은 시니어 풀스택/네트워크 엔지니어입니다. 아래 요구사항을 충족하는 OTT 동영상 스트리밍 플랫폼을 2025-12-10까지 완성하세요.

목표
- 웹 브라우저(Chrome)에서 동영상을 시청할 수 있는 OTT 서비스 구현
- 상용 MP4 파일을 서버가 보유(파일 + DB 메타데이터 관리)
- 멀티유저 고속 스트리밍(HTTP Range 206), 이어보기, 시작 위치 재생, 썸네일 자동 추출(FFmpeg)

핵심 기능
- 회원 인증: ID/PASSWORD 로그인(HTTP Basic), 토큰/세션 미구현
- 동영상 목록: 제목/썸네일/재생 버튼 표시
- 스트리밍: HTTP Range 206 Partial Content, bytes 단위 범위 응답
- 이어보기: 시청 위치 저장/복원, ‘이어보기’ vs ‘처음부터’ 선택
- 시작 위치 재생: ?start=초 또는 Range로 오프셋 시작
- 썸네일: FFmpeg로 대표 이미지 추출 후 목록에 표시
- 동시 접속: 최소 2유저 이상 원활 스트리밍, 멀티프로세스/스레드/이벤트 루프 활용

비기능 요구사항
- 안정성: 파일 잠금/에러 처리, 부분 요청 중단 대응
- 보안: 비밀번호 해시(bcrypt). 인증은 개발 단계에서 HTTP Basic 사용(프로덕션은 TLS 필수)
- 성능: 대용량 파일 스트리밍, 제로-카피/스트리밍 파이프라인, 범위 요청 캐시 고려
- 확장성: 워커(썸네일 생성), 프로세스 클러스터링
- 관측: 액세스 로그, 주요 메트릭(처리량, 초기버퍼링 시간)

산출물
- API 명세서(아래 샘플 준수)
- 데이터베이스 스키마(아래 샘플 준수)
- 기술 스택/아키텍처 설명
- Makefile 또는 프로젝트 빌드 스크립트
- 간단한 웹 UI(HTML/CSS/JS): index.html 로그인, 목록/재생/이어보기
- 테스트 시나리오/스크린샷, 최소 2명 동시 재생 성공

구현 지침
- 스트리밍: Range 헤더 파싱, 206/Content-Range/Accept-Ranges/Content-Length 엄수
- 시작 위치: Range 계산 또는 /stream?start=630(=10분30초) 허용
- 이어보기: video timeupdate 이벤트로 주기적(progress) 저장, 로그인 시 복원
- 썸네일: FFmpeg로 5초 지점 또는 10% 지점 1프레임 추출, 320px 폭 리사이즈
- 동시성: 프로세스 클러스터(예: Node.js cluster/PM2, Uvicorn workers 등)로 CPU 코어 활용
- 파일 IO: 스트림 기반(readStream), backpressure 처리
- 안전한 비번 저장: bcrypt, 로그인 Brute-force rate-limit

완료 정의(DoD)
- API e2e 테스트 통과, 2명 이상 동시 스트리밍 정상
- 대용량 MP4 파일에서 206 Range 완전 동작
- 로그인→목록→재생→진행도 저장→재로그인 후 이어보기까지 시연 가능
- README/보고서/스크린샷 구비

API/DB/스택은 아래 샘플을 기본으로 사용하라. 필요 시 합리적 범위 내 변경 가능하나, 변경 이유를 문서화하라.
— 복사 끝 —

---

## 2) 기술 스택(제안: C 언어 + SQLite)

- 백엔드: C 언어 기반 HTTP 서버
  - 웹 서버 라이브러리 후보: Mongoose(MIT 일부 상업 라이선스 주의), CivetWeb(MIT), libmicrohttpd(LGPL)
  - OS 이벤트: macOS 기준 kqueue, 또는 간단히 스레드 풀+블로킹 IO
  - 파일 전송 최적화: sendfile(2) 사용(범위 오프셋 지원) 또는 read/write 스트리밍
- DB: SQLite3 (경량 임베디드, 단일 파일, 트랜잭션/외래키 지원)
  - C API: `sqlite3_open`, `sqlite3_prepare_v2`, `sqlite3_bind_*`, `sqlite3_step`
- JSON: cJSON 또는 Parson
- 인증 암호화: libsodium(권장) 또는 bcrypt 구현(crypt_blowfish 등)
- 썸네일: FFmpeg
  - 간단: ffmpeg/ffprobe CLI를 `fork/exec`로 호출
  - 확장: libavformat/libavcodec/libswscale 직접 연동
- 네트워킹: 개발 단계는 HTTP만 지원(HTTPS/Reverse Proxy는 확장 고려사항으로 이관)
- 프론트엔드: 바닐라 HTML/CSS/JS, `<video>` 태그 사용
- 로깅: 경량 로거(예: zlog) 또는 stdout 구조 로그(JSON)
- 부하/테스트: k6, autocannon, ab(wrk 대체)

대안 스택(참고)
- Node.js/Express 또는 FastAPI(Uvicorn), Go net/http 등은 대안으로 고려 가능하나 본 프로젝트는 C/SQLite를 표준으로 함

---

## 3) API 명세서(샘플)

인증 방식(개발 단계)
- HTTP Basic Auth 사용: 모든 보호된 /api/** 요청에 `Authorization: Basic base64(id:password)` 필요
- 서버는 비밀번호를 bcrypt 해시로 저장하고, 요청 시 해시 검증만 수행(세션/토큰 없음)
- 선택(편의용): POST /api/auth/check 로 자격 검증만 수행하고 아무 토큰도 발급하지 않음

### 인증(옵션)
- POST /api/auth/check
  - req: { "id": "string", "password": "string" }
  - res 200: { "ok": true, "user": { "id": "...", "displayName": "..." } }
  - 실패: 401

### 동영상 카탈로그
- GET /api/videos?query=&page=1&pageSize=20
  - res 200: { "items":[{ "id","title","durationSec","thumbnailUrl" }], "page","pageSize","total" }
- GET /api/videos/:id
  - res 200: { "id","title","description","durationSec","mimeType","thumbnailUrl","files":[{ "path","size","bitrate"}] }
- GET /api/videos/:id/thumbnail
  - res 200: image/jpeg or image/png

### 스트리밍
- GET /api/videos/:id/stream
  - 요청 헤더: Range: bytes=start-end (예: bytes=1000- 또는 bytes=1000-2000)
  - 쿼리 파라미터(대안/보조): start=초 (예: start=630)
  - 응답:
    - 206 Partial Content + Accept-Ranges: bytes
    - Content-Range: bytes start-end/total
    - Content-Length: length
    - Content-Type: video/mp4
    - 바디: 지정 범위 바이트 스트림
  - Range 미제공 시: 200 또는 206로 초기 청크 반환(권장: 206로 통일)

### 시청 이력/이어보기
- GET /api/users/me/history?videoId=:id
  - res 200: { "videoId","lastPositionSec", "updatedAt" } 또는 204(no content)
- POST /api/videos/:id/progress
  - req: { "positionSec": number, "completed": boolean? }
  - res 204 (upsert)

### 추천(선택)
- GET /api/videos/:id/recommendations
  - res 200: { "items":[{ "id","title","thumbnailUrl" }...] }

### 오류 포맷(예시)
- res 4xx/5xx: { "error": { "code":"string","message":"string" } }

보안
- 현재 단계: HTTP 통신만 지원. Basic Auth 자격 증명은 개발 네트워크 내에서만 사용(외부 노출 금지)
- 확장 시: HTTPS(TLS), HSTS, 리다이렉트(http→https), 프록시(Nginx) 도입 권장
- Rate limit: 로그인 시도/보호 엔드포인트에 IP/계정 기준 제한

---

## 4) 데이터베이스 스키마(SQLite 버전)

특징: 단일 파일 DB, 외래키 사용 시 `PRAGMA foreign_keys=ON;` 필요. UUID는 텍스트(36)로 저장.

```sql
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;          -- 동시 읽기 성능 향상
PRAGMA synchronous = NORMAL;       -- 성능/안정성 절충
PRAGMA temp_store = MEMORY;

-- 사용자
CREATE TABLE users (
  id            TEXT PRIMARY KEY,            -- UUID 문자열
  login_id      TEXT NOT NULL UNIQUE,
  password_hash TEXT NOT NULL,
  display_name  TEXT NOT NULL,
  created_at    TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at    TEXT NOT NULL DEFAULT (datetime('now'))
);

-- 동영상 메타
CREATE TABLE videos (
  id            TEXT PRIMARY KEY,
  title         TEXT NOT NULL,
  description   TEXT,
  duration_sec  INTEGER CHECK (duration_sec >= 0),
  mime_type     TEXT DEFAULT 'video/mp4',
  created_at    TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at    TEXT NOT NULL DEFAULT (datetime('now'))
);

-- 실제 파일 (멀티 비트레이트 대비)
CREATE TABLE video_files (
  id           TEXT PRIMARY KEY,
  video_id     TEXT NOT NULL,
  file_path    TEXT NOT NULL,
  file_size    INTEGER CHECK (file_size >= 0),
  bitrate_kbps INTEGER,
  resolution   TEXT,
  created_at   TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (video_id) REFERENCES videos(id) ON DELETE CASCADE
);

-- 썸네일
CREATE TABLE thumbnails (
  id         TEXT PRIMARY KEY,
  video_id   TEXT NOT NULL,
  file_path  TEXT NOT NULL,
  width      INTEGER,
  height     INTEGER,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (video_id) REFERENCES videos(id) ON DELETE CASCADE
);

-- 시청 이력(이어보기)
CREATE TABLE watch_history (
  id                TEXT PRIMARY KEY,
  user_id           TEXT NOT NULL,
  video_id          TEXT NOT NULL,
  last_position_sec INTEGER NOT NULL DEFAULT 0,
  completed         INTEGER NOT NULL DEFAULT 0,  -- 0/1 bool
  updated_at        TEXT NOT NULL DEFAULT (datetime('now')),
  UNIQUE (user_id, video_id),
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
  FOREIGN KEY (video_id) REFERENCES videos(id) ON DELETE CASCADE
);

-- 인덱스
CREATE INDEX idx_videos_title ON videos(title);
CREATE INDEX idx_watch_history_user_video ON watch_history(user_id, video_id);
CREATE INDEX idx_video_files_video ON video_files(video_id);
```

마이그레이션 전략: 초기 단일 SQL 스크립트 → 변경 시 버전 디렉터리(`db/migrations/V1__init.sql`, `V2__add_auth_tokens.sql` 등) 관리.

---

## 5) 아키텍처 개요 (C + SQLite)

- Web(HTML/CSS/JS): 로그인 → 목록(썸네일/제목/재생) → 재생(이어보기)
- C HTTP/Streaming Server:
  - 메인 이벤트 루프(kqueue) 또는 스레드풀(accept + worker threads)
  - 전송: Range 지원 sendfile / 부분 파일 read
  - 인증: HTTP Basic 검증만 수행(세션/토큰 미사용)
  - JSON 직렬화: cJSON
- 썸네일/미디어 워커:
  - FFmpeg CLI 호출을 별도 작업 큐로 처리(스레드 안전)
  - 추출된 이미지 파일은 `media/thumbnails/`에 저장 후 DB 메타 업데이트
- DB(SQLite): 메타데이터/시청 이력/토큰 저장
- Storage: 로컬 `media/videos/` (확장 시 NFS/S3)
- Reverse Proxy: 개발 단계에서는 미사용. 추후 HTTPS/TLS 종료 및 캐시를 위해 도입 가능(확장 고려사항 참조)
- 로깅: 비동기 큐 → 파일 롤링(logrotate) 또는 단순 stdout

---

## 6) 모듈/레이어 설명 (C 구현 관점)

- http_server.c / http_router.c
  - 소켓 초기화(listen), accept 루프
  - 요청 파싱(메서드/경로/헤더/Range)
  - 라우팅 테이블(문자열 비교 또는 해시)
- auth.c
  - 패스워드 해시/검증(bcrypt 또는 libsodium pwhash)
  - 세션/토큰 관리 없음
  - 미들웨어: Authorization Basic 헤더 파싱 → id/비밀번호 추출 → 해시 검증 후 사용자 컨텍스트 주입
- json.c (래퍼)
  - 요청 바디 파싱 → 구조체
  - 응답 구조체 → JSON 문자열
- media_catalog.c
  - videos, video_files, thumbnails 조회 조합
  - 캐싱(단기: in-memory LRU) 가능
- streaming.c
  - Range 파싱: bytes=START-END
  - 파일 사이즈(stat) → 유효성 검사
  - sendfile 또는 pread 루프(Chunk: 64KB)
  - 오류 처리: 범위 불가 시 416
- watch_history.c
  - upsert (INSERT OR REPLACE)
  - 최근 시청 위치 반환
- thumbnail.c
  - ffprobe duration 추출
  - 오프셋 계산(고정 5초 혹은 duration*0.1)
  - ffmpeg -ss OFFSET -i input -vframes 1 -vf scale=320:-1 output
  - DB thumbnails INSERT
- util/uuid.c
  - UUIDv4 생성(랜덤 바이트 → 포맷)
- db.c
  - SQLite 연결 풀(필요 시) 또는 단일 연결 mutex 보호
  - 준비된 문(statement) 캐싱

에러/로깅 레벨: DEBUG/INFO/WARN/ERROR 구조체.

---

## 7) 범위 요청 처리(요약 로직)

- 요청에 Range 없으면: 200 또는 206로 시작 청크 제공(권장 206)
- Range: bytes=start-end
  - start/end 파싱, 파일 크기(total) 확인
  - 유효성: start < total, end = min(end, total-1)
  - 206 + Content-Range: bytes start-end/total, Content-Length: end-start+1
  - fs.createReadStream(file, {start, end}) 파이프

---

## 8) 웹 UI 동작 포인트

- 로그인 폼 → 성공 시 토큰 쿠키 세팅
- 목록 페이지: /api/videos 호출 → 썸네일 img src=/api/videos/:id/thumbnail
- 재생 페이지: <video src="/api/videos/:id/stream"> Range 자동 활용
- 이어보기: 페이지 진입 시 /history 조회 → 모달(이어보기/처음부터)
- 진행 저장: video timeupdate에서 5~10초마다 debounce해 /progress POST

---

## 9) 성능/테스트 전략

- 동시 2명 이상 스트리밍: 같은 파일/다른 파일 케이스
- 지표: 초기 재생 시작 시간, 버퍼 언더런 빈도, 서버 CPU/IO 사용률
- 툴: autocannon 또는 k6로 /stream 병렬 Range 소량 테스트
- 대용량 파일(>1GB)에서 206 정확성 검증
- 임계: 네트워크 끊김/중단 시 스트림 종료/로그 처리

---

## 10) 프로젝트 구조(제안: C 버전)

- server-c/
  - include/
    - http.h
    - auth.h
    - streaming.h
    - db.h
    - media.h
    - watch_history.h
    - thumbnail.h
    - json.h
    - uuid.h
  - src/
    - main.c (초기화, 설정 로드, 서버 스타트)
    - http_server.c / http_router.c
    - auth.c
    - streaming.c
    - media_catalog.c
    - watch_history.c
    - thumbnail.c
    - db.c
    - json.c (cJSON 래퍼)
    - uuid.c
    - util.c
  - third_party/ (cJSON, bcrypt/lib, optional)
  - migrations/ (V1__init.sql 등)
  - Makefile
  - config.example.ini
- web/
  - index.html (로그인)
  - videos.html (목록)
  - player.html (재생/이어보기)
  - assets/styles.css
  - assets/app.js
- media/
  - videos/ (원본 MP4)
  - thumbnails/ (생성 이미지)
- docs/
  - OTT-Spec.md
  - report.md (시스템 구조도/모듈 설명/테스트 결과/기여도)
- scripts/
  - generate_thumbnail.sh
  - load_test.sh

빌드 타깃 예시(Makefile):
```
CC=clang
CFLAGS=-O2 -Wall -Wextra -std=c17 -pthread
LIBS=-lsqlite3
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

all: ott_server

ott_server: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

clean:
	rm -f src/*.o ott_server

run: all
	./ott_server
```

---

## 11) Makefile 타깃(개요: C + SQLite)

- make build: C 서버 바이너리 빌드(clang/gcc)
- make run: 서버 실행(환경변수/ini 적용)
- make clean: 오브젝트/바이너리 정리
- make db-init: `sqlite3 app.db < migrations/V1__init.sql`
- make thumb: FFmpeg로 일괄 썸네일 생성(스크립트 호출)
- make test: 유닛/간단 통합 테스트(예: CMocka, 또는 쉘 스모크 테스트)

---

## 12) 추가 메모

- FFmpeg 설치(macOS): `brew install ffmpeg`
- 파일 권한: 대용량 파일 읽기 권한 확인
- MIME: 기본 mp4, 필요 시 webm 등 확장 가능
- 법적: 상용 MP4 사용 시 라이선스/저작권 준수

---

## 13) 제출물 체크리스트

- [ ] 프로젝트 보고서 (시스템 구조도, 함수/모듈 설명, 구현/테스트 결과, 기여도)
- [ ] 소스코드 전체 (build 가능한 Makefile/프로젝트 파일 포함)
- [ ] 웹 UI(index.html 로그인, 목록, 재생/이어보기)
- [ ] 성능 테스트(2명 이상 동시 접속) 스크린샷/로그
- [ ] API 명세/DB 스키마/기술 스택 문서 최신화

---

## 14) 확장 고려사항 (인증/인가 향후 로드맵)

현재 단계에서는 비밀번호 해시 + Basic Auth만 구현. 추후 보안/확장 필요 시 아래 기능을 단계적으로 도입:

1. 세션 토큰 (서버 상태 기반)
  - 로그인 시 난수 세션 ID 발급, Secure/HttpOnly 쿠키 저장
  - 세션 저장소: SQLite → 메모리 LRU → Redis 전환
2. JWT (Stateless)
  - Access(짧은 만료) + Refresh(긴 만료) \* HMAC-SHA256
  - 키 롤링(kid) / 블랙리스트(로그아웃, 강제 만료)
3. 권한(Authorization)
  - 역할(Role: admin/user) 또는 비디오 소유자 정책
  - 정책 표현: RBAC 테이블 또는 비트마스크 플래그
4. 보안 강화
  - Rate limiting(로그인/IP) / Fail2ban
  - CSRF(쿠키 세션 전환 시) / Content-Security-Policy
  - Argon2id(pwhash) 전환, password pepper (환경변수)
5. 감사/감시
  - 로그인 성공/실패 감사 로그
  - 비정상 다중 실패 알림(Webhook/Slack)

이 섹션은 구현 범위 외 문서화 목적.

### 네트워킹/배포 확장
- HTTPS 도입
  - 서버 인증서(ACME/Let’s Encrypt), 키 관리, 자동 갱신
  - HSTS, TLS 버전/암호군 정책, http→https 리다이렉트
- Reverse Proxy(Nginx)
  - TLS 종단, 정적 파일/썸네일 캐싱, gzip/br 압축
  - Range 요청 패스스루(Proxy 요청/응답 헤더 유지)
  - 접속 로그/지표(응답 코드별, 대역폭)
- 성능
  - sendfile + TCP_CORK/_NOPUSH(플랫폼별), keep-alive 조정
  - 커넥션 제한/스레드풀 크기 튜닝, 소켓 백로그
