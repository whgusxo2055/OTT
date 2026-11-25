// CMS API 시험용 서버 (Node.js)
// Spring Boot 버전(cms/)과 주소·응답 형식이 같다. Spring 빌드가 막혔을 때 대신 띄워 볼 수 있다.
//   실행:  cd dev-api-node && npm install && node server.js   (설정은 상위 폴더의 .env)
'use strict';
const http = require('http');
const { timingSafeEqual } = require('node:crypto');
const fs = require('fs');
const path = require('path');
const { Pool } = require('pg');
const { S3Client, GetObjectCommand } = require('@aws-sdk/client-s3');
const { getSignedUrl } = require('@aws-sdk/s3-request-presigner');

// ---- 설정 (.env → 환경 변수) ----
const envFile = process.env.OTT_ENV_FILE || path.join(__dirname, '..', '.env');
if (fs.existsSync(envFile)) for (const line of fs.readFileSync(envFile, 'utf8').split('\n')) {
  const m = line.match(/^\s*([A-Z0-9_]+)\s*=\s*(.*)\s*$/);
  if (m && !(m[1] in process.env)) process.env[m[1]] = m[2].trim().replace(/^["']|["']$/g, '');
}
const env = (k, d) => process.env[k] || d;
const PORT = Number(env('PORT', 8080));
const BUCKET = env('NCP_BUCKET', 'acme');
const URL_TTL_SEC = Number(env('POSTER_URL_TTL_MINUTES', env('PLAYBACK_URL_TTL_MINUTES', 60))) * 60;
const CMS_PLAYBACK_TOKEN = env('CMS_PLAYBACK_TOKEN', '');
const MOCKUP_DIR = path.resolve(env('MOCKUP_DIR', path.join(__dirname, '..', 'mockup', 'dist')));

const db = new Pool({ host: env('DB_HOST', 'localhost'), port: Number(env('DB_PORT', 5432)),
  database: env('DB_NAME', 'ott'), user: env('DB_USER', 'ott'), password: env('DB_PASSWORD', '') });
const s3 = new S3Client({ endpoint: env('NCP_ENDPOINT', 'https://kr.object.ncloudstorage.com'),
  region: env('NCP_REGION', 'kr-standard'), forcePathStyle: true,
  credentials: { accessKeyId: env('NCP_ACCESS_KEY'), secretAccessKey: env('NCP_SECRET_KEY') } });

const presign = key => getSignedUrl(s3, new GetObjectCommand({ Bucket: BUCKET, Key: key }), { expiresIn: URL_TTL_SEC });

// ---- 응답 도우미 ----
const CORS = { 'Access-Control-Allow-Origin': '*', 'Access-Control-Allow-Methods': 'GET, HEAD, OPTIONS' };
const json = (res, status, body) => { res.writeHead(status, { ...CORS, 'Content-Type': 'application/json; charset=utf-8' }); res.end(JSON.stringify(body)); };
const notFound = (res, message) => json(res, 404, { status: 404, message });
const UUID = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;

const COLUMNS = `id, title, description, genre_name, release_year, age_rating, actors, tags, runtime_sec,
                 video_width, video_height, published_at, hls_key, poster_key`;
const toDto = r => ({
  id: r.id, title: r.title, description: r.description, genre: r.genre_name, releaseYear: r.release_year,
  ageRating: r.age_rating, actors: r.actors, tags: r.tags, runtimeSec: r.runtime_sec,
  videoWidth: r.video_width, videoHeight: r.video_height, publishedAt: r.published_at,
  posterUrl: r.poster_key ? `/api/contents/${r.id}/poster` : null,
});
// 노출 기준은 항상 available_movie 뷰 (공개 + 미삭제 + 판권 기간 안 + 인코딩 완료)
const findAvailable = async id => (await db.query(`SELECT ${COLUMNS} FROM available_movie WHERE id = $1`, [id])).rows[0];

async function handle(req, res) {
  const url = new URL(req.url, 'http://localhost');
  const p = url.pathname;
  if (req.method === 'OPTIONS') { res.writeHead(204, CORS); return res.end(); }
  if (req.method !== 'GET' && req.method !== 'HEAD') return json(res, 405, { status: 405, message: 'GET만 지원합니다.' });
  let m;

  if (p === '/api/health') {
    const { rows } = await db.query('SELECT count(*)::int AS n FROM available_movie');
    return json(res, 200, { status: 'ok', availableContents: rows[0].n });
  }

  // 목록 (FR-12): 최신 공개순, 페이지 단위
  if (p === '/api/contents') {
    const page = Math.max(0, parseInt(url.searchParams.get('page') || '0', 10) || 0);
    const size = Math.min(500, Math.max(1, parseInt(url.searchParams.get('size') || '50', 10) || 50));
    const total = (await db.query('SELECT count(*)::int AS n FROM available_movie')).rows[0].n;
    const { rows } = await db.query(
      `SELECT ${COLUMNS} FROM available_movie ORDER BY published_at DESC, id LIMIT $1 OFFSET $2`, [size, page * size]);
    return json(res, 200, { items: rows.map(toDto), page, size, total, hasNext: (page + 1) * size < total });
  }

  // 상세 (FR-15)
  if ((m = p.match(/^\/api\/contents\/([^/]+)$/))) {
    const row = UUID.test(m[1]) && await findAvailable(m[1]);
    return row ? json(res, 200, toDto(row)) : notFound(res, '볼 수 없는 콘텐츠입니다.');
  }

  // 포스터: 버킷의 임시 서명 주소로 넘겨 준다
  if ((m = p.match(/^\/api\/contents\/([^/]+)\/poster$/))) {
    const row = UUID.test(m[1]) && await findAvailable(m[1]);
    if (!row || !row.poster_key) return notFound(res, '포스터가 없습니다.');
    res.writeHead(302, { ...CORS, Location: await presign(row.poster_key), 'Cache-Control': 'private, max-age=300' });
    return res.end();
  }

  if ((m = p.match(/^\/internal\/playback\/([^/]+)$/))) {
    const expected = Buffer.from(`Bearer ${CMS_PLAYBACK_TOKEN}`);
    const authorization = Buffer.from(req.headers.authorization || '');
    if (!CMS_PLAYBACK_TOKEN.trim() || expected.length !== authorization.length || !timingSafeEqual(expected, authorization)) {
      return json(res, 403, { status: 403, message: '접근할 수 없습니다.' });
    }
    const row = UUID.test(m[1]) && await findAvailable(m[1]);
    if (!row) return notFound(res, '재생할 수 없는 콘텐츠입니다.');
    res.setHeader('Cache-Control', 'no-store');
    return json(res, 200, { contentId: row.id, runtimeSec: row.runtime_sec, hlsKey: row.hls_key });
  }

  if (p.startsWith('/api/') || p.startsWith('/internal/')) return notFound(res, '없는 API입니다.');

  // 그 외에는 목업 정적 파일
  const file = path.join(MOCKUP_DIR, p === '/' ? 'index.html' : decodeURIComponent(p));
  if (!file.startsWith(MOCKUP_DIR) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) return notFound(res, '파일이 없습니다.');
  const types = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.css': 'text/css; charset=utf-8', '.svg': 'image/svg+xml' };
  res.writeHead(200, { 'Content-Type': types[path.extname(file)] || 'application/octet-stream' });
  fs.createReadStream(file).pipe(res);
}

http.createServer((req, res) => handle(req, res).catch(e => {
  console.error(req.method, req.url, e.name, e.message);
  if (!res.headersSent) json(res, e.name === 'NoSuchKey' ? 404 : 500, { status: 500, message: '서버 오류: ' + e.message });
})).listen(PORT, () => console.log(`CMS dev API: http://localhost:${PORT}  (bucket=${BUCKET}, mockup=${MOCKUP_DIR})`));
