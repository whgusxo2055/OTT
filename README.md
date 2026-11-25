# OTT

이 저장소는 CMS와 스트리밍 서버를 별도 모듈로 관리한다. Java 21이 필요하고, 시연 영상 생성에는 FFmpeg가 필요하다. Maven Wrapper는 최초 실행 시 Maven과 프로젝트 의존성을 내려받는다. 루트에서 두 모듈을 한 번에 빌드할 수 있다.

```sh
./streaming-server/mvnw -B package
```

## 로컬 스트리밍 시연

```sh
cd streaming-server
./scripts/generate-demo.sh
./mvnw spring-boot:run
```

브라우저에서 `http://127.0.0.1:8080/`을 열고 기본 콘텐츠 ID `movie-001`로 **영상 불러오기**를 누른 뒤 재생한다. 영상이 시작되면 일시정지하고 재생 막대에서 45초로 이동할 수 있다.

- 재생 정보는 `GET /api/v1/contents/movie-001/playback`에서 확인한다.
- [Swagger UI](http://127.0.0.1:8080/swagger-ui.html)와 [OpenAPI JSON](http://127.0.0.1:8080/v3/api-docs)에서 재생 정보와 HLS API를 확인한다.
- 시연 영상은 약 75초 길이이며 `streaming-server/media/`에 생성된다.
- [아키텍처와 코드 작성 패턴](docs/architecture.md), [Git 커밋 규칙](CONTRIBUTING.md)

## CMS와 스트리밍 서버 실행

준비된 PostgreSQL의 `available_movie` 뷰와 NCP Object Storage를 사용한다. `.env.example`을 `.env`로 복사해 DB와 NCP 설정을 채우고, `CMS_PLAYBACK_TOKEN`에 충분히 긴 임의의 값을 설정한다. 실행 환경에서 두 서비스에 같은 토큰을 주입한다.

```sh
./cms/run.sh
```

CMS가 실행 중인 상태에서 별도 터미널로 빌드된 스트리밍 서버를 시작한다.

```sh
cd streaming-server
SERVER_PORT=8081 STREAMING_CMS_BASE_URL=http://127.0.0.1:8080 \
  java -jar target/streaming-server-0.1.0-SNAPSHOT.jar \
  '--spring.config.import=file:../.env[.properties]'
```

CMS 화면은 `http://127.0.0.1:8080/`에서 콘텐츠 목록을 읽고, 재생 요청은 `http://127.0.0.1:8081`로 보낸다. 스트리밍 플레이어에서 CMS 콘텐츠 UUID를 입력하거나 `/?contentId=<UUID>`로 열 수도 있다. 다른 주소에서 목업을 제공할 경우 `mockup/dist/cms-config.js`의 `STREAMING_SERVER`와 스트리밍 서버의 `STREAMING_PLAYER_ALLOWED_ORIGINS`를 설정한다.

CMS의 예전 `/api/playback/**`를 호출하는 클라이언트는 스트리밍 서버의 `/api/v1/contents/{contentId}/playback`과 응답의 `manifestUrl`을 사용한다. 두 서비스와 목업은 함께 전환한다.

## 회원 상태 서버

`user-state-server`는 8082에서 실행하며 PostgreSQL의 `user_state` 스키마를 사용한다. DB 계정은 최초 실행 시 이 스키마와 테이블을 생성할 수 있어야 한다. 개발용 회원 A·B는 고정 UUID를 사용하며, 데모 인증은 `demo` 프로필에서 활성화된다. 실행 환경에 서로 다른 임의의 `DEMO_TOKEN_A`, `DEMO_TOKEN_B`를 설정한다.

```sh
cd user-state-server
SPRING_PROFILES_ACTIVE=demo java -jar target/user-state-server.jar \
  '--spring.config.import=file:../.env[.properties]'
```

`CMS_BASE_URL`과 `STREAMING_BASE_URL`의 기본값은 각각 같은 서버의 8080, 8081이다. 목업은 `cms-config.js`의 `STATE_API_BASE`로 접속한다. 기본 허용 출처는 8080의 localhost와 127.0.0.1이며, 다른 출처는 `STATE_ALLOWED_ORIGINS`에 쉼표로 구분해 지정한다. 목업의 **시연 도구 → 기존 회원 A/B**를 선택해 해당 회원의 토큰을 입력한다. 토큰은 브라우저 탭의 세션 저장소에 둔다.

| 회원 API | 용도 |
| --- | --- |
| `GET /api/v1/me` | 현재 인증된 회원 ID 확인 |
| `GET /api/v1/me/watchlist` | 최근 찜한 순서로 공개 콘텐츠 조회 |
| `PUT / DELETE /api/v1/me/watchlist/{contentId}` | 찜 추가·해제 |
| `PUT / GET /api/v1/me/progress/{contentId}` | 밀리초 단위 시청 위치 저장·조회 |
| `GET /api/v1/me/continue-watching` | 30초 이상 시청하고 90% 미만인 작품을 최근 저장순으로 조회 |

각 요청은 `Authorization: Bearer <token>`을 사용한다. 시청 위치 요청 본문 예시는 `{"positionMs":35000}`이다. 공개 여부와 제목은 CMS, 길이는 스트리밍 서버에서 확인한다. 찜과 시청 상태의 소유자는 내부 회원 UUID로 식별한다. 소셜 로그인 도입 시 외부 제공자의 고유 계정과 내부 회원을 연결하고 `MemberResolver`에서 서비스 세션을 해석한다.
