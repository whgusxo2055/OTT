// CMS API 서버 주소 설정
//  - 목업을 CMS 서버에서 열 때 (http://서버주소:8080/) : 비워 둔다. 같은 서버의 API를 쓴다.
//  - 내 컴퓨터에서 index.html 파일을 바로 열 때        : 서버 주소를 적는다.  예) 'http://123.45.67.89:8080'
//  - 주소가 없으면 CMS에 연결하지 않고 예전처럼 내장 샘플 데이터로 동작한다.
const CMS_SERVER = '';

window.CMS_API_BASE = CMS_SERVER || (location.protocol === 'file:' ? null : '');

// 스트리밍 서버 주소. 별도 도메인이나 게이트웨이를 쓰면 이 값을 설정한다.
const STREAMING_SERVER = '';
const cmsOrigin = new URL(CMS_SERVER || location.href);
window.STREAMING_API_BASE = STREAMING_SERVER || (
  cmsOrigin.hostname ? `${cmsOrigin.protocol}//${cmsOrigin.hostname}:8081` : null
);
