# Nginx 리버스 프록시

백엔드 세 서비스와 Nginx가 같은 호스트에서 실행되는 구성을 기준으로 한다. Nginx는 목업 정적 파일을 제공하고 기존 API 경로를 그대로 각 백엔드로 전달한다.

| 요청 경로 | 대상 |
| --- | --- |
| `/`, 정적 파일 | `mockup/dist` 배포본 |
| `/api/contents`, `/api/contents/**` | CMS 8080 |
| `/api/v1/contents/{id}/playback`, `/media/**` | 스트리밍 8081 |
| `/api/v1/me`, `/api/v1/me/**` | 회원 상태 8082 |

`ott.conf`는 루프백 8090에서 실행하는 개발용 설정이다. `mockup/dist`의 내용을 설정의 `root` 디렉터리에 배치하고, 이 파일을 Nginx의 `http` 컨텍스트에 포함한 뒤 `nginx -t`를 실행한다. 컨테이너에서 Nginx만 실행하는 경우 백엔드 주소를 컨테이너에서 접근 가능한 주소로 바꾼다.

개발 서버 `ott-dev`에 적용한 뒤에는 SSH 터널로 접속한다.

```sh
ssh -N -L 8090:127.0.0.1:8090 ott-dev
```

브라우저에서 `http://127.0.0.1:8090/`을 연다. 이 주소에서 화면과 세 API를 함께 사용한다.

Nginx가 제공하는 `/cms-config.js`는 세 API의 기준 주소를 현재 출처로 지정한다. 플레이어가 받은 `/media/**` 경로는 같은 출처에서 그대로 사용한다. 회원 인증의 `Authorization`과 HLS의 `Range` 헤더는 업스트림에 전달된다. `Host`의 포트를 유지해 Spring이 브라우저의 `Origin`을 같은 출처로 판단하게 한다. HLS 응답은 버퍼링을 끄고 회원 상태 응답에는 별도 프록시 캐시를 적용하지 않는다. `/internal/**`는 프록시에서 404를 반환한다.

외부 공개 시에는 실제 도메인과 TLS를 설정하고, Spring이 신뢰할 수 있는 Nginx에서 전달한 `X-Forwarded-Proto`를 사용하도록 구성한다. 이후 Nginx가 유일한 외부 진입점이 되도록 백엔드 포트의 바인딩 및 방화벽 규칙을 확인한다. 회원 A·B 토큰은 개발 환경에서 사용한다. CMS 포스터는 CMS가 발급한 Object Storage 서명 URL로 브라우저가 리다이렉트된다.
