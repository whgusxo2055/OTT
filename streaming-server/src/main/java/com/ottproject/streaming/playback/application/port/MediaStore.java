package com.ottproject.streaming.playback.application.port;

import java.io.IOException;
import java.util.Optional;
import org.springframework.core.io.Resource;

public interface MediaStore {

    Optional<Resource> find(String key) throws IOException;
}
