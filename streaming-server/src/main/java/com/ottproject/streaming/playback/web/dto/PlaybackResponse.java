package com.ottproject.streaming.playback.web.dto;

public record PlaybackResponse(String contentId, String manifestUrl, long durationMs) {}
