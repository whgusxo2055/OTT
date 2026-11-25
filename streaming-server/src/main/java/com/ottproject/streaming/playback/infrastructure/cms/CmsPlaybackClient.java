package com.ottproject.streaming.playback.infrastructure.cms;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URI;
import java.util.Optional;
import java.util.UUID;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.stereotype.Component;

@Component
@ConditionalOnProperty("streaming.cms.base-url")
class CmsPlaybackClient {

    private static final int MAX_DOCUMENT_BYTES = 1024 * 1024;

    private final URI baseUrl;
    private final String token;

    CmsPlaybackClient(@Value("${streaming.cms.base-url}") URI baseUrl,
                      @Value("${CMS_PLAYBACK_TOKEN:}") String token) {
        if (!("http".equalsIgnoreCase(baseUrl.getScheme()) || "https".equalsIgnoreCase(baseUrl.getScheme()))
                || baseUrl.getHost() == null || baseUrl.getRawUserInfo() != null
                || baseUrl.getRawQuery() != null || baseUrl.getRawFragment() != null
                || !(baseUrl.getPath().isEmpty() || baseUrl.getPath().equals("/"))) {
            throw new IllegalArgumentException("streaming.cms.base-url must be an HTTP(S) origin");
        }
        if (token.isBlank()) {
            throw new IllegalArgumentException("CMS_PLAYBACK_TOKEN is required for CMS playback");
        }
        this.baseUrl = baseUrl;
        this.token = token;
    }

    Optional<byte[]> readDocument(String path) throws IOException {
        var result = open(path);
        if (result.isEmpty()) {
            return Optional.empty();
        }

        var connection = result.get();
        try (var stream = connection.getInputStream()) {
            var bytes = stream.readNBytes(MAX_DOCUMENT_BYTES + 1);
            if (bytes.length > MAX_DOCUMENT_BYTES) {
                throw new IOException("CMS playback document exceeds the size limit");
            }
            return Optional.of(bytes);
        } finally {
            connection.disconnect();
        }
    }

    private Optional<HttpURLConnection> open(String path) throws IOException {
        var connection = (HttpURLConnection) baseUrl.resolve(path).toURL().openConnection();
        try {
            connection.setRequestProperty("Authorization", "Bearer " + token);
            connection.setInstanceFollowRedirects(false);
            connection.setConnectTimeout(3_000);
            connection.setReadTimeout(15_000);
            int status = connection.getResponseCode();
            if (status == HttpURLConnection.HTTP_OK) {
                return Optional.of(connection);
            }
            if (status == HttpURLConnection.HTTP_NOT_FOUND) {
                connection.disconnect();
                return Optional.empty();
            }

            throw new IOException("CMS playback API returned HTTP " + status);
        } catch (IOException exception) {
            connection.disconnect();
            throw exception;
        }
    }

    static boolean isCmsId(String contentId) {
        try {
            return UUID.fromString(contentId).toString().equalsIgnoreCase(contentId);
        } catch (IllegalArgumentException exception) {
            return false;
        }
    }
}
