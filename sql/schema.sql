-- =====================================================================
-- OTT CMS 콘텐츠 스키마 - Phase 1 v1.2 (PostgreSQL 14 이상)
-- 범위: 영화 한 편을 등록 → 업로드 → 인코딩 → 공개 → 목록·상세·재생
-- 관련 FR: FR-05, 07, 09, 10, 12, 15 (+ FR-16 재생 권한에서 노출 기준 공유)
-- =====================================================================

CREATE TYPE publish_status AS ENUM ('PRIVATE', 'PUBLIC');
CREATE TYPE asset_type     AS ENUM ('SOURCE_VIDEO', 'POSTER');
CREATE TYPE job_status     AS ENUM ('WAITING', 'PROCESSING', 'DONE', 'FAILED');

-- ---------- 1. 장르 (코드표) ----------
CREATE TABLE genre (
    id    SMALLSERIAL PRIMARY KEY,
    name  VARCHAR(30) NOT NULL UNIQUE          -- '드라마', 'SF', ... (오타·표기 차이 방지)
);

-- ---------- 2. 콘텐츠 (영화) ----------
CREATE TABLE content (
    id              UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    title           VARCHAR(200)   NOT NULL,
    description     TEXT,
    genre_id        SMALLINT       NOT NULL REFERENCES genre(id),
    release_year    SMALLINT       NOT NULL CHECK (release_year BETWEEN 1900 AND 2100),
    age_rating      VARCHAR(10)    NOT NULL CHECK (age_rating IN ('ALL', '12', '15', '19')),
    actors          TEXT,                      -- Phase 1은 표시용 문자열 (검색은 Phase 2)
    tags            TEXT,                      -- 〃
    runtime_sec     INTEGER        CHECK (runtime_sec > 0),

    -- 수급 정보 (수급처 미확정 → 문자열로 최소한만)
    source          VARCHAR(50)    NOT NULL DEFAULT 'SYNTHETIC',  -- 'SYNTHETIC'(자체 생성), 'ADMIN'(관리자 직접 등록)
    external_id     VARCHAR(100),                                 -- 수급처의 원래 ID (예: C00001)
    license_start   TIMESTAMPTZ,               -- 판권 시작 (NULL = 제한 없음)
    license_end     TIMESTAMPTZ,               -- 판권 종료 (NULL = 제한 없음)

    -- 상태
    publish_status  publish_status NOT NULL DEFAULT 'PRIVATE',   -- 처음엔 항상 비공개 (FR-10)
    published_at    TIMESTAMPTZ,
    active_job_id   UUID,                      -- 지금 재생에 쓰는 인코딩 결과
    poster_asset_id UUID,                      -- 지금 화면에 쓰는 포스터 (홈·목록·상세 카드)
    created_at      TIMESTAMPTZ    NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ    NOT NULL DEFAULT now(),
    deleted_at      TIMESTAMPTZ,               -- 소프트 삭제 (FR-05)

    UNIQUE (source, external_id),              -- 같은 데이터를 두 번 등록해도 중복 안 됨
    CHECK (license_end IS NULL OR license_start IS NULL OR license_end > license_start),
    -- 공개하려면 재생할 영상과 공개 시각이 있어야 함 (FR-10)
    CHECK (publish_status <> 'PUBLIC' OR (active_job_id IS NOT NULL AND published_at IS NOT NULL))
);
CREATE INDEX idx_content_list ON content (published_at DESC)
    WHERE publish_status = 'PUBLIC' AND deleted_at IS NULL;      -- 사용자 목록 (FR-12)

-- ---------- 3. 미디어 파일 (파일은 Storage, 여기는 위치만) ----------
CREATE TABLE media_asset (
    id           UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    content_id   UUID         NOT NULL REFERENCES content(id),
    asset_type   asset_type   NOT NULL,        -- 원본 영상 / 포스터
    storage_key  VARCHAR(500) NOT NULL UNIQUE, -- 'original/C00001.mp4', 'poster/C00001.jpg'
    mime_type    VARCHAR(100) NOT NULL,        -- 'video/mp4', 'video/quicktime'(MOV), 'image/jpeg'
    size_bytes   BIGINT       CHECK (size_bytes >= 0),
    width        INTEGER,                      -- 영상 가로 해상도 (입수할 때 ffprobe로 확인)
    height       INTEGER,                      -- 영상 세로 해상도 (720 / 1080 / 2160)
    duration_sec NUMERIC(9,2),                 -- 영상 길이(초)
    created_at   TIMESTAMPTZ  NOT NULL DEFAULT now()
);
CREATE INDEX idx_asset_content ON media_asset (content_id, asset_type);

-- ---------- 4. 인코딩 작업 = 작업 큐 (FR-09) ----------
CREATE TABLE encoding_job (
    id               UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    source_asset_id  UUID       NOT NULL REFERENCES media_asset(id),
    status           job_status NOT NULL DEFAULT 'WAITING',
    output_key       VARCHAR(500),             -- 'hls/{contentId}/{jobId}/index.m3u8'
    error_message    TEXT,
    retry_count      SMALLINT   NOT NULL DEFAULT 0,
    worker_id        VARCHAR(100),
    started_at       TIMESTAMPTZ,
    finished_at      TIMESTAMPTZ,
    created_at       TIMESTAMPTZ NOT NULL DEFAULT now(),
    CHECK (status <> 'DONE' OR output_key IS NOT NULL)
);
CREATE INDEX idx_job_waiting ON encoding_job (created_at) WHERE status = 'WAITING';

ALTER TABLE content ADD FOREIGN KEY (active_job_id)   REFERENCES encoding_job(id);
ALTER TABLE content ADD FOREIGN KEY (poster_asset_id) REFERENCES media_asset(id);

-- ---------- 5. 노출 기준 (목록·상세·재생 권한 공통) ----------
CREATE VIEW available_movie AS
SELECT c.*, g.name AS genre_name,
       j.output_key  AS hls_key,               -- HLS 재생목록 위치
       p.storage_key AS poster_key,            -- 포스터 위치
       v.width       AS video_width,           -- 원본 해상도
       v.height      AS video_height
FROM content c
JOIN genre g        ON g.id = c.genre_id
JOIN encoding_job j ON j.id = c.active_job_id AND j.status = 'DONE'
JOIN media_asset v  ON v.id = j.source_asset_id
LEFT JOIN media_asset p ON p.id = c.poster_asset_id
WHERE c.publish_status = 'PUBLIC'
  AND c.deleted_at IS NULL
  AND (c.license_start IS NULL OR c.license_start <= now())
  AND (c.license_end   IS NULL OR now() < c.license_end);
