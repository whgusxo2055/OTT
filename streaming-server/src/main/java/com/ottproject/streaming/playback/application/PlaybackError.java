package com.ottproject.streaming.playback.application;

public enum PlaybackError {
    CONTENT_NOT_FOUND("콘텐츠를 찾을 수 없습니다."),
    SEGMENT_NOT_FOUND("영상 조각을 찾을 수 없습니다."),
    PLAYBACK_NOT_READY("재생 목록이 준비되지 않았습니다."),
    MEDIA_UNAVAILABLE("영상 파일을 읽을 수 없습니다.");

    private final String message;

    PlaybackError(String message) {
        this.message = message;
    }

    public String message() {
        return message;
    }
}
