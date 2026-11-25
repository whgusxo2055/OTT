package com.ottproject.streaming.playback.infrastructure;

import com.ottproject.streaming.playback.application.port.MediaStore;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Optional;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.io.FileSystemResource;
import org.springframework.core.io.Resource;
import org.springframework.stereotype.Component;

@Component
public class LocalMediaStore implements MediaStore {

    private final Path root;

    LocalMediaStore(@Value("${streaming.media-root}") String mediaRoot) {
        root = Path.of(mediaRoot).toAbsolutePath().normalize();
    }

    @Override
    public Optional<Resource> find(String key) {
        var path = root.resolve(key).normalize();
        if (!path.startsWith(root) || !Files.isRegularFile(path) || !Files.isReadable(path)) {
            return Optional.empty();
        }
        return Optional.of(new FileSystemResource(path));
    }
}
