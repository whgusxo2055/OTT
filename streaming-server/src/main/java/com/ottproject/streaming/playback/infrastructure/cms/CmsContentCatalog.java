package com.ottproject.streaming.playback.infrastructure.cms;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.ottproject.streaming.playback.application.port.ContentCatalog;
import com.ottproject.streaming.playback.infrastructure.DemoContentCatalog;
import java.io.IOException;
import java.util.Optional;
import java.util.UUID;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.context.annotation.Primary;
import org.springframework.stereotype.Component;

@Component
@Primary
@ConditionalOnProperty("streaming.cms.base-url")
class CmsContentCatalog implements ContentCatalog {

    static final String CMS_MEDIA_PREFIX = "ncp:";

    private final CmsPlaybackClient cms;
    private final ObjectMapper mapper;
    private final DemoContentCatalog demo;

    CmsContentCatalog(CmsPlaybackClient cms, ObjectMapper mapper, DemoContentCatalog demo) {
        this.cms = cms;
        this.mapper = mapper;
        this.demo = demo;
    }

    @Override
    public Optional<PlayableContent> find(String contentId) throws IOException {
        if (!CmsPlaybackClient.isCmsId(contentId)) {
            return demo.find(contentId);
        }

        var id = UUID.fromString(contentId);
        var document = cms.readDocument("/internal/playback/" + id);
        if (document.isEmpty()) {
            return Optional.empty();
        }

        var info = mapper.readValue(document.get(), CmsPlaybackInfo.class);
        if (info == null || !id.equals(info.contentId()) || !validKey(id, info.hlsKey())) {
            throw new IOException("CMS returned unexpected playback metadata");
        }

        long durationMs = info.runtimeSec() == null ? 0 : info.runtimeSec() * 1000L;
        return Optional.of(new PlayableContent(contentId, durationMs, CMS_MEDIA_PREFIX + info.hlsKey()));
    }

    private boolean validKey(UUID contentId, String key) {
        String prefix = "hls/" + contentId + "/";
        return key != null && key.startsWith(prefix)
                && key.substring(prefix.length()).matches("[0-9a-fA-F-]{36}/index\\.m3u8");
    }

    private record CmsPlaybackInfo(UUID contentId, Integer runtimeSec, String hlsKey) {}
}
