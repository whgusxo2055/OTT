package com.ottproject.streaming.playback.infrastructure;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.ottproject.streaming.playback.application.port.ContentCatalog;
import java.io.IOException;
import java.util.Optional;
import org.springframework.core.io.ClassPathResource;
import org.springframework.stereotype.Component;

@Component
public class DemoContentCatalog implements ContentCatalog {

    private final PlayableContent content;

    DemoContentCatalog(ObjectMapper mapper) throws IOException {
        try (var input = new ClassPathResource("demo-content.json").getInputStream()) {
            content = mapper.readValue(input, PlayableContent.class);
        }
    }

    @Override
    public Optional<PlayableContent> find(String contentId) {
        return content.contentId().equals(contentId) ? Optional.of(content) : Optional.empty();
    }
}
