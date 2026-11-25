package com.ottproject.cms.content;

import java.time.OffsetDateTime;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

import org.springframework.jdbc.core.RowMapper;
import org.springframework.jdbc.core.simple.JdbcClient;
import org.springframework.stereotype.Repository;

/**
 * 사용자에게 보여 줄 콘텐츠 조회.
 * 항상 available_movie 뷰만 읽는다 → 공개 + 미삭제 + 판권 기간 안 + 인코딩 완료 조건이 한곳(DB 뷰)에서 지켜진다.
 */
@Repository
public class ContentRepository {

    private static final String COLUMNS = """
            id, title, description, genre_name, release_year, age_rating, actors, tags, runtime_sec,
            video_width, video_height, published_at, hls_key, poster_key""";

    private static final RowMapper<ContentRow> MAPPER = (rs, rowNum) -> new ContentRow(
            rs.getObject("id", UUID.class),
            rs.getString("title"),
            rs.getString("description"),
            rs.getString("genre_name"),
            rs.getInt("release_year"),
            rs.getString("age_rating"),
            rs.getString("actors"),
            rs.getString("tags"),
            rs.getObject("runtime_sec", Integer.class),
            rs.getObject("video_width", Integer.class),
            rs.getObject("video_height", Integer.class),
            rs.getObject("published_at", OffsetDateTime.class),
            rs.getString("hls_key"),
            rs.getString("poster_key"));

    private final JdbcClient jdbc;

    public ContentRepository(JdbcClient jdbc) {
        this.jdbc = jdbc;
    }

    /** 최신 공개순 한 페이지 (FR-12) */
    public List<ContentRow> findPage(int page, int size) {
        return jdbc.sql("SELECT " + COLUMNS + " FROM available_movie"
                        + " ORDER BY published_at DESC, id LIMIT :size OFFSET :offset")
                .param("size", size)
                .param("offset", (long) page * size)
                .query(MAPPER)
                .list();
    }

    public long count() {
        return jdbc.sql("SELECT count(*) FROM available_movie").query(Long.class).single();
    }

    /** 노출 가능한 콘텐츠 한 건. 비공개·삭제·판권 만료·인코딩 전이면 비어 있다. */
    public Optional<ContentRow> findById(UUID id) {
        return jdbc.sql("SELECT " + COLUMNS + " FROM available_movie WHERE id = :id")
                .param("id", id)
                .query(MAPPER)
                .optional();
    }
}
