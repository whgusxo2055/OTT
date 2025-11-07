# OTT 서버 테스트 정보

## 🌐 서버 접속 정보
- **URL**: http://localhost:8080
- **상태**: ✅ 실행 중

## 👥 테스트 계정

### 계정 1
- **아이디**: test
- **비밀번호**: password
- **이름**: 테스트 사용자

### 계정 2
- **아이디**: test123
- **비밀번호**: Asdqwe123!
- **이름**: 테스트 사용자

## 📊 현재 데이터베이스 상태
- 사용자: 2명
- 동영상: 0개

## 🧪 테스트 단계

### 1단계: 로그인 테스트 ✅
1. 브라우저에서 http://localhost:8080 접속
2. 위의 계정으로 로그인
3. 로그인 성공 후 videos.html로 리디렉션 확인

### 2단계: 동영상 추가 (필요시)
```bash
# 테스트 동영상이 있다면
./scripts/add_video.sh /path/to/video.mp4 '테스트 영상' '설명'
```

### 3단계: 스트리밍 테스트
- 동영상 목록에서 영상 클릭
- 재생 확인
- Seek 기능 테스트
- 이어보기 기능 테스트

### 4단계: API 테스트
```bash
# 인증 확인
curl -u test:password http://localhost:8080/api/auth/check

# 동영상 목록
curl -u test:password http://localhost:8080/api/videos

# 시청 기록
curl -u test:password http://localhost:8080/api/users/me/history
```

## 📝 테스트 체크리스트
- [ ] 로그인 페이지 접속
- [ ] HTTP Basic Auth 로그인
- [ ] 동영상 목록 페이지 로딩
- [ ] 검색 기능
- [ ] 페이지네이션
- [ ] 동영상 재생
- [ ] Seek 기능 (Range 206)
- [ ] 이어보기 기능
- [ ] 시청 기록 저장
- [ ] 동시 접속 테스트 (2명 이상)

