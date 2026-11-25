package com.ottproject.state;

import java.util.Map;
import org.springframework.http.ResponseEntity;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

class StateFailure extends RuntimeException {
    final int status;
    final String code;

    StateFailure(int status, String code, String message) {
        super(message);
        this.status = status;
        this.code = code;
    }
}

@RestControllerAdvice
class StateFailureHandler {
    @ExceptionHandler(StateFailure.class)
    ResponseEntity<?> failure(StateFailure e) {
        return ResponseEntity.status(e.status).body(Map.of("error", Map.of("code", e.code, "message", e.getMessage())));
    }

    @ExceptionHandler(HttpMessageNotReadableException.class)
    ResponseEntity<?> invalidJson() {
        return failure(new StateFailure(400, "INVALID_REQUEST", "JSON 요청을 확인하세요."));
    }
}
