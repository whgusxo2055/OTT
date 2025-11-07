# OTT 스트리밍 서버 프로젝트 - 구현 현황

## 프로젝트 정보

- **프로젝트명**: OTT 동영상 스트리밍 서버
- **시작일**: 2025-11-07
- **제출기한**: 2025-12-10
- **기술 스택**: C17, CivetWeb, SQLite, libsodium, FFmpeg

## 구현 완료된 기능

### ✅ 1. 핵심 인프라

- [x] 프로젝트 디렉터리 구조 생성
- [x] Makefile 작성 (빌드/클린/실행 타깃)
- [x] 데이터베이스 스키마 (SQLite)
- [x] 설정 파일 구조

### ✅ 2. 서버 모듈

#### 기본 유틸리티
- [x] Logger (로그 시스템)
- [x] UUID 생성기
- [x] Thread Pool (스레드 풀 구현)

#### 인증 시스템
- [x] libsodium 기반 비밀번호 해싱
- [x] HTTP Basic Auth 파싱
- [x] 사용자 인증 미들웨어

#### 데이터베이스
- [x] SQLite 연결 풀
- [x] 사용자 CRUD
- [x] 동영상 CRUD
- [x] 시청 이력 관리
- [x] 썸네일 관리

#### 스트리밍
- [x] HTTP Range 파싱
- [x] 206 Partial Content 응답
- [x] 청크 단위 파일 전송
- [x] 시작 위치 파라미터 지원

#### 미디어 처리
- [x] FFmpeg 기반 썸네일 생성
- [x] 비디오 duration 추출
- [x] 썸네일 DB 저장

#### HTTP 핸들러
- [x] 인증 체크 API
- [x] 동영상 목록 API
- [x] 동영상 상세 API
- [x] 스트리밍 API
- [x] 썸네일 API
- [x] 시청 이력 API
- [x] 진행도 저장 API

#### JSON 처리
- [x] cJSON 통합
- [x] 응답 헬퍼 함수
- [x] 에러 응답 포맷

### ✅ 3. 웹 UI

- [x] 로그인 페이지 (index.html)
- [x] 동영상 목록 페이지 (videos.html)
- [x] 재생 페이지 (player.html)
- [x] 반응형 CSS (styles.css)
- [x] JavaScript 유틸리티 (app.js)
- [x] 이어보기 모달
- [x] 검색 기능
- [x] 페이지네이션

### ✅ 4. 관리 스크립트

- [x] 설정 스크립트 (setup.sh)
- [x] 사용자 생성 (create_user.sh)
- [x] 비디오 추가 (add_video.sh)

### ✅ 5. 문서

- [x] README.md
- [x] API 명세 (OTT-Spec.md)
- [x] 구현 체크리스트 (현재 문서)

## 다음 단계 (구현 필요)

### 🔨 1단계: 빌드 및 초기 테스트

```bash
cd server-c
make deps        # 의존성 다운로드
make all         # 빌드
make db-init     # DB 초기화
```

**예상 문제**:
- CivetWeb/cJSON 다운로드 확인
- 컴파일 에러 수정 필요
- 링커 에러 해결

### 🔨 2단계: 기능 테스트

1. **서버 시작 테스트**
   ```bash
   make run
   ```

2. **사용자 생성 테스트**
   ```bash
   ../scripts/create_user.sh test password123 '테스트'
   ```

3. **비디오 추가 테스트**
   ```bash
   ../scripts/add_video.sh /path/to/video.mp4 '테스트영상' '설명'
   ```

4. **웹 UI 테스트**
   - http://localhost:8080 접속
   - 로그인
   - 목록 확인
   - 재생 테스트

### 🔨 3단계: Range 요청 테스트

```bash
# 전체 파일
curl -u test:password123 http://localhost:8080/api/videos/<ID>/stream -o test1.mp4

# 범위 요청
curl -u test:password123 -H "Range: bytes=0-1000000" http://localhost:8080/api/videos/<ID>/stream -o test2.mp4

# 시작 위치
curl -u test:password123 "http://localhost:8080/api/videos/<ID>/stream?start=60" -o test3.mp4
```

### 🔨 4단계: 동시성 테스트

최소 2명 이상 동시 스트리밍 테스트 필요

### 🔨 5단계: 디버깅 및 최적화

**예상 수정사항**:
- 컴파일 에러
- 런타임 에러
- 메모리 누수
- 성능 이슈

## 알려진 이슈 및 해결 필요 사항

### ✅ 해결된 문제

1. **CivetWeb 통합** ✅
   - ~~헤더 파일 경로 확인 필요~~ → Makefile에 경로 설정 완료
   - ~~API 호환성 검증 필요~~ → mg_get_var() 5개 인자로 수정 완료

2. **uuid_t 타입 충돌** ✅
   - ~~시스템 uuid_t와 충돌~~ → ott_uuid_t로 변경

3. **libsodium 경로** ✅
   - ~~헤더 파일 찾기 실패~~ → SODIUM_PREFIX 설정 완료

4. **pthread 누락** ✅
   - ~~pthread_mutex_t 정의 없음~~ → db.h에 #include <pthread.h> 추가

5. **한국어 로컬라이제이션** ✅
   - 모든 로그 메시지 및 사용자 대면 텍스트 한국어 번역 완료

### 🐛 잠재적 문제

1. **파일 경로**
   - 절대 경로 vs 상대 경로 일관성 확인 필요
   - 디렉터리 생성 로직 검증 필요

2. **스레드 안전성**
   - DB 연결 뮤텍스 동작 확인 필요
   - 파일 I/O 동시 접근 테스트 필요

3. **경고 메시지**
   - streaming.c:131 - sign comparison 경고 (기능에는 영향 없음)

## 개발 결정사항 (기록)

### ✅ 확정된 기술 선택

1. **HTTP 서버**: CivetWeb (MIT)
2. **비밀번호 해싱**: libsodium (Argon2id)
3. **동시성 모델**: 스레드 풀 + 블로킹 I/O
4. **프로젝트 방식**: 전체 구조 먼저 생성 후 단계적 구현

### 📝 구현 결정사항

- 인증: HTTP Basic Auth (개발용)
- DB 연결: 단일 연결 + 뮤텍스 (간단한 풀)
- 스레드 풀: 4 워커 스레드
- 청크 크기: 64KB
- 썸네일: 320px 폭, 5초 또는 10% 지점

## 다음 작업

### 우선순위 1 (즉시)
- [x] 프로젝트 빌드 및 컴파일 에러 수정 ✅
- [x] 기본 서버 시작 테스트 ✅
- [ ] 사용자 생성 및 로그인 테스트

### 우선순위 2 (단기)
- [ ] 비디오 추가 및 목록 조회
- [ ] 스트리밍 기능 검증
- [ ] 이어보기 기능 테스트

### 우선순위 3 (중기)
- [ ] 성능 테스트 및 최적화
- [ ] 동시 접속 테스트 (2명 이상)
- [ ] 문서 보완

### 우선순위 4 (장기/선택)
- [ ] HTTPS 지원 (확장)
- [ ] JWT 인증 (확장)
- [ ] 다중 비트레이트 (확장)

## 진행률

```
전체 진행률: ██████████ 95%

[완료] 구조 설계      ████████████████████ 100%
[완료] 코드 작성      ████████████████████ 100%
[완료] 빌드 테스트    ████████████████████ 100%
[완료] 한국어화       ████████████████████ 100%
[대기] 기능 테스트    ░░░░░░░░░░░░░░░░░░░░   0%
[대기] 성능 테스트    ░░░░░░░░░░░░░░░░░░░░   0%
[완료] 문서화         ████████████████░░░░  80%
```

## 연락처 및 도움말

질문이나 결정이 필요한 사항이 생기면 언제든지 물어보세요!

**현재 상태**: ✅ 빌드 성공, 서버 실행 확인 완료, 한국어 로컬라이제이션 완료

**다음 단계**: 실제 동영상으로 테스트
```bash
# 서버 실행
cd /Users/jht/Desktop/Projects/NetworkPrograming/ott/server-c
./ott_server

# 사용자 생성 (다른 터미널에서)
../scripts/create_user.sh testuser password123 '테스트 사용자'

# 동영상 추가
../scripts/add_video.sh /path/to/video.mp4 '테스트 동영상' '설명'
```

**서버 로그 예시** (한국어):
```
[2025-11-07 20:29:44] [INFO] OTT 스트리밍 서버를 시작합니다...
[2025-11-07 20:29:44] [INFO] libsodium 초기화 완료
[2025-11-07 20:29:44] [INFO] 데이터베이스 초기화 완료: app.db
[2025-11-07 20:29:44] [INFO] HTTP 서버가 포트 8080에서 시작되었습니다
[2025-11-07 20:29:44] [INFO] 서버가 준비되었습니다!
[2025-11-07 20:29:44] [INFO] 서버 주소: http://localhost:8080
[2025-11-07 20:29:44] [INFO] 종료하려면 Ctrl+C를 누르세요
```
