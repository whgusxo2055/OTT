-- 인코딩이 끝난 비공개 영화를 한꺼번에 공개 (관리자의 일괄 공개 작업, FR-10)
-- 공개 조건: 재생할 영상(active_job_id)이 있고 그 작업이 DONE
UPDATE content c
SET publish_status = 'PUBLIC',
    published_at   = COALESCE(c.published_at, now()),   -- 처음 공개한 시각 유지
    updated_at     = now()
FROM encoding_job j
WHERE j.id = c.active_job_id AND j.status = 'DONE'
  AND c.publish_status = 'PRIVATE' AND c.deleted_at IS NULL;
