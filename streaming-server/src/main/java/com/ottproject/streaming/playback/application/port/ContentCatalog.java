package com.ottproject.streaming.playback.application.port;

import java.io.IOException;
import java.util.Optional;

public interface ContentCatalog {

    Optional<PlayableContent> find(String contentId) throws IOException;

    record PlayableContent(String contentId, long durationMs, String playlist) {}
}
