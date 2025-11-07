# OTT 동영상 스트리밍 서버

C 언어 기반 HTTP Range 206을 지원하는 고성능 OTT 스트리밍 서버입니다.

## 프로젝트 개요

- **언어**: C17
- **웹 서버**: CivetWeb (MIT License)
- **데이터베이스**: SQLite3
- **인증**: libsodium (Argon2id password hashing)
- **미디어 처리**: FFmpeg
- **동시성**: 스레드 풀 + 블로킹 I/O

## 주요 기능

✅ **HTTP Range 206 지원** - 부분 요청으로 효율적인 스트리밍  
✅ **멀티스레드** - 스레드 풀 기반 동시 접속 처리  
✅ **이어보기** - 시청 위치 저장 및 복원  
✅ **시작 위치 재생** - `?start=초` 파라미터 지원  
✅ **자동 썸네일** - FFmpeg 기반 자동 추출  
✅ **보안 인증** - HTTP Basic Auth + Argon2id 해싱  
✅ **반응형 웹 UI** - 모바일/데스크톱 지원  

## 시스템 요구사항

- macOS (테스트 환경)
- Xcode Command Line Tools
- Homebrew

## 설치 방법

### 1. 시스템 의존성 설치

```bash
# Homebrew를 통한 설치
brew install sqlite3 libsodium ffmpeg
```

### 2. 프로젝트 설정

```bash
# 설정 스크립트 실행
chmod +x scripts/*.sh
./scripts/setup.sh
```

### 3. 빌드

```bash
cd server-c
make deps    # 서드파티 라이브러리 다운로드
make all     # 서버 빌드
```

## 사용 방법

### 서버 실행

```bash
cd server-c
make run
```

서버가 `http://localhost:8080`에서 실행됩니다.

### 사용자 생성

```bash
./scripts/create_user.sh <아이디> <비밀번호> <이름>

# 예제
./scripts/create_user.sh admin password123 '관리자'
```

### 동영상 추가

```bash
./scripts/add_video.sh <비디오파일> <제목> <설명>

# 예제
./scripts/add_video.sh ~/Downloads/movie.mp4 '영화 제목' '영화 설명'
```

### 웹 UI 접속

1. 브라우저에서 `http://localhost:8080` 접속
2. 생성한 계정으로 로그인
3. 동영상 목록에서 재생

## API 문서

### 인증

모든 API 요청에는 HTTP Basic Authentication 헤더가 필요합니다:

```
Authorization: Basic base64(username:password)
```

### 엔드포인트

#### `POST /api/auth/check`
사용자 인증 확인

#### `GET /api/videos?page=1&pageSize=20&query=검색어`
동영상 목록 조회

#### `GET /api/videos/:id`
동영상 상세 정보

#### `GET /api/videos/:id/stream`
동영상 스트리밍 (Range 지원)

**헤더**:
- `Range: bytes=start-end` (선택)

**쿼리**:
- `start=초` (선택) - 시작 위치 지정

#### `GET /api/videos/:id/thumbnail`
썸네일 이미지

#### `GET /api/users/me/history?videoId=:id`
시청 이력 조회

#### `POST /api/videos/:id/progress`
시청 진행도 저장

**Body**:
```json
{
  "positionSec": 120,
  "completed": false
}
```

## 프로젝트 구조

```
ott/
├── server-c/           # C 서버 소스
│   ├── include/        # 헤더 파일
│   ├── src/           # 구현 파일
│   ├── third_party/   # 서드파티 라이브러리
│   ├── migrations/    # DB 마이그레이션
│   └── Makefile       # 빌드 파일
├── web/               # 웹 UI
│   ├── index.html     # 로그인 페이지
│   ├── videos.html    # 동영상 목록
│   ├── player.html    # 재생 페이지
│   └── assets/        # CSS/JS
├── media/             # 미디어 파일
│   ├── videos/        # 동영상 파일
│   └── thumbnails/    # 썸네일 이미지
├── scripts/           # 유틸리티 스크립트
└── docs/             # 문서
```

## 개발 가이드

### 빌드 타깃

```bash
make all        # 전체 빌드
make clean      # 빌드 산출물 삭제
make clean-all  # 모든 파일 삭제 (의존성 포함)
make deps       # 서드파티 라이브러리 다운로드
make db-init    # 데이터베이스 초기화
make db-reset   # 데이터베이스 재설정
make run        # 서버 실행
```

### 로그 레벨

`logger.c`에서 로그 레벨 설정:
- `LOG_DEBUG` - 상세 디버그 정보
- `LOG_INFO` - 일반 정보
- `LOG_WARN` - 경고
- `LOG_ERROR` - 에러

### 스레드 풀 설정

`config.h`에서 스레드 수 조정:
```c
#define SERVER_THREADS 4
```

## 성능 테스트

### 동시 접속 테스트

```bash
# 2명 이상 동시 스트리밍 테스트
# 터미널 1
curl -u admin:password123 -r 0-1000000 http://localhost:8080/api/videos/<ID>/stream -o /dev/null

# 터미널 2
curl -u admin:password123 -r 1000000-2000000 http://localhost:8080/api/videos/<ID>/stream -o /dev/null
```

## 보안 고려사항

⚠️ **개발 단계 전용**

현재 구현은 개발/테스트 목적입니다:

- HTTP Basic Auth 사용 (평문 전송)
- HTTPS 미지원
- Rate limiting 미구현

**프로덕션 배포 시**:
- HTTPS/TLS 필수
- Reverse Proxy (Nginx) 사용
- Rate limiting 구현
- 강화된 인증 (JWT 등)

## 트러블슈팅

### ffmpeg를 찾을 수 없음
```bash
brew install ffmpeg
```

### libsodium 링크 에러
```bash
brew reinstall libsodium
```

### 데이터베이스 잠금 에러
```bash
make db-reset
```

## 라이선스

이 프로젝트는 교육 목적으로 작성되었습니다.

**서드파티 라이선스**:
- CivetWeb: MIT License
- cJSON: MIT License
- SQLite: Public Domain
- libsodium: ISC License

## 기여자

- 개발자: [이름]
- 제출일: 2025-12-10

## 참고 자료

- [CivetWeb Documentation](https://github.com/civetweb/civetweb)
- [SQLite C API](https://www.sqlite.org/c3ref/intro.html)
- [libsodium Documentation](https://doc.libsodium.org/)
- [FFmpeg Documentation](https://ffmpeg.org/documentation.html)
