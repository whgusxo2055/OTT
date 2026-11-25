# OTT 서비스 아키텍처

## 책임과 데이터 흐름

- CMS는 원본 등록, FFmpeg 변환 작업, HLS 저장, 콘텐츠 공개 상태와 재생본 정보를 관리한다. 브라우저에는 콘텐츠 목록·상세·포스터를 제공한다.
- 스트리밍 서버는 콘텐츠 ID로 재생 정보를 조회하고 플레이어에 HLS 재생 목록과 영상 조각을 제공한다.
- 찜·이어보기 서버는 회원 ID와 콘텐츠 ID별 찜과 시청 위치를 관리한다. 별도 서비스로 구현할 예정이다.

브라우저는 스트리밍 서버의 재생 정보 API를 호출하고, 반환된 재생 목록과 영상 조각을 같은 서버에서 받는다. 스트리밍 서버는 루프백 주소에서 실행하며 재생 요청은 인증 없이 받는다.

`STREAMING_CMS_BASE_URL`을 설정하면 스트리밍 서버가 CMS의 `/internal/playback/{id}`에서 공개되어 재생 가능한 콘텐츠의 길이와 HLS 오브젝트 키를 조회한다. 이 API는 두 서버에 설정한 `CMS_PLAYBACK_TOKEN`으로 인증한다. 스트리밍 서버는 NCP Object Storage에서 HLS 목록과 조각을 직접 읽고 기존 재생 API로 전달한다. 로컬 시연 ID는 테스트 JSON과 로컬 HLS 파일에서 계속 읽는다.

CMS가 제공하는 목업에서도 재생 정보와 HLS는 스트리밍 서버로 요청한다. 스트리밍 서버에서 브라우저의 CMS 출처는 `STREAMING_PLAYER_ALLOWED_ORIGINS`로 허용한다. 스트리밍 서버의 NCP 자격 증명에는 HLS 오브젝트 조회 권한이 필요하다.

## 코드 작성 패턴

- Java 21, Spring Boot MVC, Maven Wrapper를 사용한다. 기능별 패키지를 만들고, HTTP 매핑은 컨트롤러에, 콘텐츠 조회와 재생 가능 여부 판단은 서비스 경계에 둔다.
- 생성자로 의존성을 주입한다. 응답과 테스트 콘텐츠 모델은 불변 record로 정의한다. 재생 오류는 `PlaybackError`에 정의하고, `PlaybackExceptionHandler`에서 HTTP 상태와 기존 `error.code`, `error.message` 응답으로 변환한다.
- 재생 정보와 HLS API의 OpenAPI 태그, 파라미터, 응답은 각 컨트롤러의 Swagger 어노테이션에서 관리한다.
- 콘텐츠 ID와 재생 목록에 등록된 영상 조각 이름으로만 미디어를 조회한다. 요청 경로를 파일시스템 경로로 그대로 사용하지 않는다.
- 영상 조각은 스트림으로 전달한다. 시간 단위는 API 경계에서 밀리초로 통일한다.

스트리밍 서버의 `playback` 패키지는 다음과 같이 구성한다.

| 패키지 | 책임 |
| --- | --- |
| `web` | 재생 정보와 HLS HTTP 엔드포인트 |
| `web.dto`, `web.error` | 응답 모델과 HTTP 오류 변환 |
| `application` | 재생 정보 조회, 재생 목록 확인, 영상 조각 조회 |
| `application.port` | 콘텐츠 조회 및 미디어 저장소 인터페이스 |
| `infrastructure` | 데모 JSON, 로컬 HLS 저장소, CMS 메타데이터 조회, NCP HLS 저장소 |

## 모듈 추가 방식

저장소는 서비스별 최상위 디렉터리를 사용한다. `cms/`와 `streaming-server/`는 루트 Maven 프로젝트에 등록되며 각각 실행하고 배포할 수 있다. `sql/`, `tools/`, `scripts/`는 CMS가 사용하는 스키마와 콘텐츠 처리 도구를, `mockup/`은 CMS가 제공하는 목업을 담는다. 후속 모듈은 각 서비스가 소유하는 데이터와 인터페이스를 기준으로 연결한다.

스트리밍 서버의 `ContentCatalog`는 재생 가능한 콘텐츠의 ID·길이·재생 목록 키를 조회한다. `MediaStore`는 키에 대응하는 미디어를 조회한다. 로컬과 CMS 조회 구현은 같은 재생 API 계약과 브라우저 주소 체계를 사용한다.
