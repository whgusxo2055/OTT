package com.ottproject.streaming.playback.application;

public final class PlaybackException extends RuntimeException {

    private final PlaybackError error;

    PlaybackException(PlaybackError error) {
        super(error.message());
        this.error = error;
    }

    public PlaybackError error() {
        return error;
    }
}
