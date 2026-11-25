# 협업 규칙

- 작업 브랜치는 `codex/<작업명>`처럼 목적을 드러낸다.
- 커밋 제목은 `type(scope): 변경 요약`을 사용한다. 한 커밋은 독립적으로 설명할 수 있는 변경 한 가지를 담는다.
- 유형은 `feat`, `fix`, `docs`, `refactor`, `test`, `build`, `chore`를 사용한다. 서비스 작업의 범위는 `streaming`, `user-state`처럼 모듈 이름으로 적는다.
- 예시는 `feat(streaming): 로컬 HLS 재생 API 추가`, `docs(architecture): 재생 서버 책임 정의`다.
- 커밋 전 관련 검증 결과와 변경 목록을 확인한다. 실행 중 생성된 영상, 빌드 산출물, 실제 키와 비밀번호는 Git에서 제외한다.
