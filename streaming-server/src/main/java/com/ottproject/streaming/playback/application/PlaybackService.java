package com.ottproject.streaming.playback.application;

import com.ottproject.streaming.playback.application.port.ContentCatalog;
import com.ottproject.streaming.playback.application.port.MediaStore;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.regex.Pattern;
import org.springframework.core.io.Resource;
import org.springframework.stereotype.Service;

@Service
public class PlaybackService {

    private static final Pattern SEGMENT_NAME = Pattern.compile("[A-Za-z0-9_.-]+\\.ts");
    private static final String SEGMENT_DURATION_PREFIX = "#EXTINF:";

    private final ContentCatalog catalog;
    private final MediaStore media;

    PlaybackService(ContentCatalog catalog, MediaStore media) {
        this.catalog = catalog;
        this.media = media;
    }

    public PlaybackInfo playback(String contentId) throws IOException {
        var content = content(contentId);
        var manifest = playlist(content.playlist());
        long durationMs = content.durationMs() > 0 ? content.durationMs() : durationFrom(manifest);
        return new PlaybackInfo(contentId, durationMs);
    }

    public byte[] manifest(String contentId) throws IOException {
        return playlist(content(contentId).playlist()).getContentAsByteArray();
    }

    public Resource segment(String contentId, String name) throws IOException {
        var playlistKey = content(contentId).playlist();
        if (!SEGMENT_NAME.matcher(name).matches()
                || playlist(playlistKey).getContentAsString(StandardCharsets.UTF_8).lines().noneMatch(name::equals)) {
            throw new PlaybackException(PlaybackError.SEGMENT_NOT_FOUND);
        }

        var segmentKey = playlistKey.substring(0, playlistKey.lastIndexOf('/') + 1) + name;
        return media.find(segmentKey)
                .orElseThrow(() -> new PlaybackException(PlaybackError.SEGMENT_NOT_FOUND));
    }

    private ContentCatalog.PlayableContent content(String contentId) throws IOException {
        return catalog.find(contentId).orElseThrow(() ->
                new PlaybackException(PlaybackError.CONTENT_NOT_FOUND));
    }

    private Resource playlist(String key) throws IOException {
        return media.find(key).orElseThrow(() ->
                new PlaybackException(PlaybackError.PLAYBACK_NOT_READY));
    }

    private long durationFrom(Resource manifest) throws IOException {
        try {
            double seconds = manifest.getContentAsString(StandardCharsets.UTF_8).lines()
                    .filter(line -> line.startsWith(SEGMENT_DURATION_PREFIX))
                    .map(line -> line.substring(SEGMENT_DURATION_PREFIX.length()).split(",", 2)[0])
                    .mapToDouble(Double::parseDouble)
                    .sum();
            if (!Double.isFinite(seconds) || seconds <= 0 || seconds >= Long.MAX_VALUE / 1000.0) {
                throw new IOException("HLS playlist does not provide a valid duration");
            }
            return Math.round(seconds * 1000);
        } catch (NumberFormatException exception) {
            throw new IOException("HLS playlist contains an invalid segment duration");
        }
    }

    public record PlaybackInfo(String contentId, long durationMs) {}
}
