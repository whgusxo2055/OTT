package com.ottproject.cms.content;

import java.time.OffsetDateTime;
import java.util.UUID;

/** 화면에 내려 주는 콘텐츠 정보. 버킷 경로는 넣지 않고, 포스터는 API 주소로 알려 준다. */
public record ContentResponse(
        UUID id,
        String title,
        String description,
        String genre,
        int releaseYear,
        String ageRating,
        String actors,
        String tags,
        Integer runtimeSec,
        Integer videoWidth,
        Integer videoHeight,
        OffsetDateTime publishedAt,
        String posterUrl) {

    public static ContentResponse from(ContentRow row) {
        String posterUrl = row.posterKey() == null ? null : "/api/contents/" + row.id() + "/poster";
        return new ContentResponse(row.id(), row.title(), row.description(), row.genre(), row.releaseYear(),
                row.ageRating(), row.actors(), row.tags(), row.runtimeSec(), row.videoWidth(), row.videoHeight(),
                row.publishedAt(), posterUrl);
    }
}
