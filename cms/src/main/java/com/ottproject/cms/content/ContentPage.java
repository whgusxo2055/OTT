package com.ottproject.cms.content;

import java.util.List;

/** 목록 응답 한 페이지 */
public record ContentPage(List<ContentResponse> items, int page, int size, long total, boolean hasNext) {
}
