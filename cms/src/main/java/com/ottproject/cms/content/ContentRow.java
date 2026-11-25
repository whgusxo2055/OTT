package com.ottproject.cms.content;

import java.time.OffsetDateTime;
import java.util.UUID;

/** available_movie 뷰의 한 행. 버킷 안의 파일 위치(hlsKey, posterKey)는 서버 안에서만 쓴다. */
public record ContentRow(
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
        String hlsKey,
        String posterKey) {}
