package com.ottproject.streaming.playback.web;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpHeaders;
import org.springframework.web.servlet.config.annotation.CorsRegistration;
import org.springframework.web.servlet.config.annotation.CorsRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

@Configuration
class PlaybackCorsConfiguration implements WebMvcConfigurer {

    private final String[] origins;

    PlaybackCorsConfiguration(
            @Value("${STREAMING_PLAYER_ALLOWED_ORIGINS:http://127.0.0.1:8080,http://localhost:8080}") String[] origins) {
        this.origins = origins;
    }

    @Override
    public void addCorsMappings(CorsRegistry registry) {
        configure(registry.addMapping("/api/v1/contents/*/playback"));
        configure(registry.addMapping("/media/**"));
    }

    private void configure(CorsRegistration registration) {
        registration.allowedOrigins(origins)
                .allowedMethods("GET", "HEAD")
                .allowedHeaders(HttpHeaders.RANGE)
                .exposedHeaders(HttpHeaders.ACCEPT_RANGES, HttpHeaders.CONTENT_RANGE);
    }
}
