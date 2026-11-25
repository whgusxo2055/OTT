package com.ottproject.state;

import com.fasterxml.jackson.databind.JsonNode;
import java.net.http.HttpClient;
import java.time.Duration;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.client.JdkClientHttpRequestFactory;
import org.springframework.stereotype.Component;
import org.springframework.web.client.RestClient;
import org.springframework.web.client.RestClientException;
import org.springframework.web.client.RestClientResponseException;

@Component
class Catalog {
    private final RestClient cms;
    private final RestClient streaming;

    Catalog(@Value("${CMS_BASE_URL:http://127.0.0.1:8080}") String cmsUrl,
            @Value("${STREAMING_BASE_URL:http://127.0.0.1:8081}") String streamUrl) {
        var factory = new JdkClientHttpRequestFactory(HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(3)).build());
        factory.setReadTimeout(Duration.ofSeconds(10));
        cms = RestClient.builder().baseUrl(cmsUrl).requestFactory(factory).build();
        streaming = RestClient.builder().baseUrl(streamUrl).requestFactory(factory).build();
    }

    String title(String id) {
        return read(cms, "/api/contents/{id}", id).required("title").asText();
    }

    long duration(String id) {
        return read(streaming, "/api/v1/contents/{id}/playback", id).required("durationMs").asLong();
    }

    private JsonNode read(RestClient client, String path, String id) {
        try {
            return client.get().uri(path, id).retrieve().body(JsonNode.class);
        } catch (RestClientResponseException e) {
            int status = e.getStatusCode().value();
            if (status == 404 || status == 400) throw new StateFailure(404, "CONTENT_NOT_FOUND", "공개 콘텐츠를 찾을 수 없습니다.");
            if (status == 409) throw new StateFailure(409, "PLAYBACK_NOT_READY", "재생 목록이 준비되지 않았습니다.");
        } catch (RestClientException e) {
            // Return the same service error for connection and response failures.
        }
        throw new StateFailure(503, "CATALOG_UNAVAILABLE", "콘텐츠 정보를 조회할 수 없습니다.");
    }
}
