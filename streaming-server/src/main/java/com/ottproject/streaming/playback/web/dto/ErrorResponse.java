package com.ottproject.streaming.playback.web.dto;

public record ErrorResponse(ErrorDetail error) {

    public record ErrorDetail(String code, String message) {}
}
