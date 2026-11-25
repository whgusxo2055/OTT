package com.ottproject.cms.playback;

import com.ottproject.cms.content.ContentRepository;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.UUID;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.CacheControl;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;

@RestController
class PlaybackMetadataController {

    private final ContentRepository contents;
    private final String token;

    PlaybackMetadataController(ContentRepository contents, @Value("${CMS_PLAYBACK_TOKEN:}") String token) {
        this.contents = contents;
        this.token = token;
    }

    @GetMapping("/internal/playback/{id}")
    ResponseEntity<PlaybackMetadata> playback(
            @PathVariable UUID id, @RequestHeader(name = HttpHeaders.AUTHORIZATION, required = false) String authorization) {
        if (token.isBlank() || authorization == null || !MessageDigest.isEqual(
                ("Bearer " + token).getBytes(StandardCharsets.UTF_8), authorization.getBytes(StandardCharsets.UTF_8))) {
            throw new ResponseStatusException(HttpStatus.FORBIDDEN);
        }

        var row = contents.findById(id).orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND));
        return ResponseEntity.ok()
                .cacheControl(CacheControl.noStore())
                .body(new PlaybackMetadata(row.id(), row.runtimeSec(), row.hlsKey()));
    }

    record PlaybackMetadata(UUID contentId, Integer runtimeSec, String hlsKey) {}
}
