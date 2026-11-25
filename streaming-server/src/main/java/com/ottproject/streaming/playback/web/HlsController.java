package com.ottproject.streaming.playback.web;

import com.ottproject.streaming.playback.application.PlaybackService;
import com.ottproject.streaming.playback.web.dto.ErrorResponse;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.Parameter;
import io.swagger.v3.oas.annotations.enums.ParameterIn;
import io.swagger.v3.oas.annotations.headers.Header;
import io.swagger.v3.oas.annotations.media.Content;
import io.swagger.v3.oas.annotations.media.Schema;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.tags.Tag;
import java.io.IOException;
import org.springframework.core.io.Resource;
import org.springframework.http.CacheControl;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping(HlsController.MEDIA_PATH)
@Tag(name = "HLS 미디어", description = "HLS 재생 목록과 영상 조각 조회")
@ApiResponse(responseCode = "409", description = "재생 목록 준비 중 (PLAYBACK_NOT_READY)",
        content = @Content(mediaType = MediaType.APPLICATION_JSON_VALUE,
                schema = @Schema(implementation = ErrorResponse.class)))
@ApiResponse(responseCode = "503", description = "미디어 I/O 실패 (MEDIA_UNAVAILABLE)",
        content = @Content(mediaType = MediaType.APPLICATION_JSON_VALUE,
                schema = @Schema(implementation = ErrorResponse.class)))
public class HlsController {

    static final String MEDIA_PATH = "/media/{contentId}";
    static final String MANIFEST_PATH = "/index.m3u8";

    private static final String HLS_PLAYLIST = "application/vnd.apple.mpegurl";
    private static final String MPEG_TS = "video/mp2t";

    private final PlaybackService playback;

    HlsController(PlaybackService playback) {
        this.playback = playback;
    }

    @Operation(summary = "HLS 재생 목록 조회")
    @ApiResponse(responseCode = "200", description = "M3U8 재생 목록",
            headers = @Header(name = HttpHeaders.CACHE_CONTROL, schema = @Schema(type = "string", example = "no-store")),
            content = @Content(mediaType = HLS_PLAYLIST, schema = @Schema(type = "string")))
    @ApiResponse(responseCode = "404", description = "재생할 콘텐츠를 찾을 수 없음 (CONTENT_NOT_FOUND)",
            content = @Content(mediaType = MediaType.APPLICATION_JSON_VALUE,
                    schema = @Schema(implementation = ErrorResponse.class)))
    @GetMapping(MANIFEST_PATH)
    ResponseEntity<byte[]> manifest(@Parameter(description = "콘텐츠 ID", example = "movie-001")
                                    @PathVariable String contentId) throws IOException {
        return ResponseEntity.ok()
                .cacheControl(CacheControl.noStore())
                .contentType(MediaType.parseMediaType(HLS_PLAYLIST))
                .body(playback.manifest(contentId));
    }

    @Operation(summary = "HLS 영상 조각 조회",
            parameters = @Parameter(name = HttpHeaders.RANGE, in = ParameterIn.HEADER,
                    description = "요청할 바이트 범위", schema = @Schema(type = "string", example = "bytes=0-1023")))
    @ApiResponse(responseCode = "200", description = "MPEG-TS 영상 조각",
            content = @Content(mediaType = MPEG_TS, schema = @Schema(type = "string", format = "binary")))
    @ApiResponse(responseCode = "206", description = "요청한 바이트 범위",
            headers = @Header(name = HttpHeaders.CONTENT_RANGE, description = "단일 범위의 위치와 전체 길이",
                    schema = @Schema(type = "string", example = "bytes 0-1023/582800")),
            content = {
                    @Content(mediaType = MPEG_TS, schema = @Schema(type = "string", format = "binary")),
                    @Content(mediaType = "multipart/byteranges", schema = @Schema(type = "string", format = "binary"))
            })
    @ApiResponse(responseCode = "404", description = "CONTENT_NOT_FOUND 또는 SEGMENT_NOT_FOUND",
            content = @Content(mediaType = MediaType.APPLICATION_JSON_VALUE,
                    schema = @Schema(implementation = ErrorResponse.class)))
    @ApiResponse(responseCode = "416", description = "처리할 수 없는 바이트 범위. 본문에는 전체 영상 조각이 포함됩니다.",
            headers = @Header(name = HttpHeaders.CONTENT_RANGE,
                    schema = @Schema(type = "string", example = "bytes */582800")),
            content = @Content(mediaType = MPEG_TS, schema = @Schema(type = "string", format = "binary")))
    @GetMapping("/{name}")
    ResponseEntity<Resource> segment(
            @Parameter(description = "콘텐츠 ID", example = "movie-001") @PathVariable String contentId,
            @Parameter(description = "재생 목록에 등록된 영상 조각 이름", example = "seg_00000.ts")
            @PathVariable String name) throws IOException {
        return ResponseEntity.ok()
                .contentType(MediaType.parseMediaType(MPEG_TS))
                .body(playback.segment(contentId, name));
    }
}
