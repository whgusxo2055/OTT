-- 현재 상태 한눈에 보기:  psql ... -f sql/status.sql
\echo '== 인코딩 작업 상태 =='
SELECT status, count(*) FROM encoding_job GROUP BY status ORDER BY status;
\echo '== 콘텐츠 공개 상태 =='
SELECT publish_status, count(*) FROM content WHERE deleted_at IS NULL GROUP BY publish_status;
\echo '== 지금 사용자에게 보이는 영화 수 (공개 + 인코딩 완료 + 판권 기간 안) =='
SELECT count(*) AS available FROM available_movie;
\echo '== 원본 해상도별 개수 · 용량 =='
SELECT height || 'p' AS resolution, count(*) AS videos,
       round(sum(duration_sec) / 3600, 1) AS hours,
       pg_size_pretty(sum(size_bytes)::bigint) AS original_size
FROM media_asset WHERE asset_type = 'SOURCE_VIDEO' GROUP BY height ORDER BY height;
