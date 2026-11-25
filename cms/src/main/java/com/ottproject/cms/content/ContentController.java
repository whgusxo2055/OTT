package com.ottproject.cms.content;

import java.net.URI;
import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.springframework.http.CacheControl;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;

import com.ottproject.cms.storage.StorageService;

/** 콘텐츠 목록(FR-12) · 상세(FR-15) · 포스터 */
@RestController
public class ContentController {

    private static final int MAX_PAGE_SIZE = 500;

    private final ContentRepository contents;
    private final StorageService storage;

    public ContentController(ContentRepository contents, StorageService storage) {
        this.contents = contents;
        this.storage = storage;
    }

    /** 서버와 DB가 살아 있는지 확인용 */
    @GetMapping("/api/health")
    public Map<String, Object> health() {
        return Map.of("status", "ok", "availableContents", contents.count());
    }

    /** 공개된 영화 목록. 최신 공개순, 페이지 단위. */
    @GetMapping("/api/contents")
    public ContentPage list(@RequestParam(name = "page", defaultValue = "0") int page,
                            @RequestParam(name = "size", defaultValue = "50") int size) {
        int safePage = Math.max(0, page);
        int safeSize = Math.min(MAX_PAGE_SIZE, Math.max(1, size));
        long total = contents.count();
        List<ContentResponse> items = contents.findPage(safePage, safeSize).stream()
                .map(ContentResponse::from)
                .toList();
        boolean hasNext = (long) (safePage + 1) * safeSize < total;
        return new ContentPage(items, safePage, safeSize, total, hasNext);
    }

    @GetMapping("/api/contents/{id}")
    public ContentResponse detail(@PathVariable("id") UUID id) {
        return ContentResponse.from(find(id));
    }

    /**
     * 포스터 이미지. 버킷은 비공개이므로 잠깐만 유효한 서명 주소로 넘겨 준다(302).
     * 브라우저의 img 태그가 이 주소를 따라가 버킷에서 이미지를 받는다.
     */
    @GetMapping("/api/contents/{id}/poster")
    public ResponseEntity<Void> poster(@PathVariable("id") UUID id) {
        ContentRow row = find(id);
        if (row.posterKey() == null) {
            throw new ResponseStatusException(HttpStatus.NOT_FOUND, "포스터가 없습니다.");
        }
        return ResponseEntity.status(HttpStatus.FOUND)
                .location(URI.create(storage.presignedUrl(row.posterKey())))
                .cacheControl(CacheControl.maxAge(Duration.ofMinutes(5)).cachePrivate())
                .build();
    }

    private ContentRow find(UUID id) {
        return contents.findById(id)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "볼 수 없는 콘텐츠입니다."));
    }
}
