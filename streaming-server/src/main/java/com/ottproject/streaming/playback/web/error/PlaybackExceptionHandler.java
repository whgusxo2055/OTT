package com.ottproject.streaming.playback.web.error;

import com.ottproject.streaming.playback.application.PlaybackError;
import com.ottproject.streaming.playback.application.PlaybackException;
import com.ottproject.streaming.playback.web.HlsController;
import com.ottproject.streaming.playback.web.PlaybackController;
import com.ottproject.streaming.playback.web.dto.ErrorResponse;
import java.io.IOException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

@RestControllerAdvice(assignableTypes = {PlaybackController.class, HlsController.class})
class PlaybackExceptionHandler {

    private static final Logger log = LoggerFactory.getLogger(PlaybackExceptionHandler.class);

    @ExceptionHandler(PlaybackException.class)
    ResponseEntity<ErrorResponse> handlePlaybackFailure(PlaybackException exception) {
        return errorResponse(exception.error());
    }

    @ExceptionHandler(IOException.class)
    ResponseEntity<ErrorResponse> handleMediaFailure(IOException exception) {
        log.warn("Failed to read or deliver playback media", exception);
        return errorResponse(PlaybackError.MEDIA_UNAVAILABLE);
    }

    private ResponseEntity<ErrorResponse> errorResponse(PlaybackError error) {
        var status = switch (error) {
            case CONTENT_NOT_FOUND, SEGMENT_NOT_FOUND -> HttpStatus.NOT_FOUND;
            case PLAYBACK_NOT_READY -> HttpStatus.CONFLICT;
            case MEDIA_UNAVAILABLE -> HttpStatus.SERVICE_UNAVAILABLE;
        };
        return ResponseEntity.status(status)
                .contentType(MediaType.APPLICATION_JSON)
                .body(new ErrorResponse(new ErrorResponse.ErrorDetail(error.name(), error.message())));
    }
}
