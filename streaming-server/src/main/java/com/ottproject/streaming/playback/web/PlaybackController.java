package com.ottproject.streaming.playback.web;

import com.ottproject.streaming.playback.application.PlaybackService;
import com.ottproject.streaming.playback.web.dto.ErrorResponse;
import com.ottproject.streaming.playback.web.dto.PlaybackResponse;
import io.swagger.v3.oas.annotations.OpenAPIDefinition;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.Parameter;
import io.swagger.v3.oas.annotations.info.Info;
import io.swagger.v3.oas.annotations.media.Content;
import io.swagger.v3.oas.annotations.media.Schema;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.tags.Tag;
import java.io.IOException;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RestController;

@RestController
@OpenAPIDefinition(info = @Info(title = "OTT 스트리밍 API", version = "v1",
        description = "콘텐츠 재생 정보와 HLS 미디어를 제공합니다. 재생 시간의 단위는 밀리초입니다."))
@Tag(name = "재생 정보", description = "콘텐츠의 HLS 재생 주소와 재생 시간 조회")
public class PlaybackController {

    private final PlaybackService playback;

    PlaybackController(PlaybackService playback) {
        this.playback = playback;
    }

    @Operation(summary = "콘텐츠 재생 정보 조회")
    @ApiResponse(responseCode = "200", description = "콘텐츠 ID, HLS 재생 목록 주소, 재생 시간(밀리초)",
            content = @Content(mediaType = MediaType.APPLICATION_JSON_VALUE,
                    schema = @Schema(implementation = PlaybackResponse.class)))
    @ApiResponse(responseCode = "404", description = "재생할 콘텐츠를 찾을 수 없음 (CONTENT_NOT_FOUND)",
            content = @Content(mediaType = MediaType.APPLICATION_JSON_VALUE,
                    schema = @Schema(implementation = ErrorResponse.class)))
    @ApiResponse(responseCode = "409", description = "재생 목록 준비 중 (PLAYBACK_NOT_READY)",
            content = @Content(mediaType = MediaType.APPLICATION_JSON_VALUE,
                    schema = @Schema(implementation = ErrorResponse.class)))
    @ApiResponse(responseCode = "503", description = "미디어 I/O 실패 (MEDIA_UNAVAILABLE)",
            content = @Content(mediaType = MediaType.APPLICATION_JSON_VALUE,
                    schema = @Schema(implementation = ErrorResponse.class)))
    @GetMapping("/api/v1/contents/{contentId}/playback")
    PlaybackResponse playback(@Parameter(description = "콘텐츠 ID", example = "movie-001")
                              @PathVariable String contentId) throws IOException {
        var info = playback.playback(contentId);
        var manifestUrl = HlsController.MEDIA_PATH.replace("{contentId}", info.contentId())
                + HlsController.MANIFEST_PATH;
        return new PlaybackResponse(info.contentId(), manifestUrl, info.durationMs());
    }
}
