package com.ottproject.state;

import com.fasterxml.jackson.databind.JsonNode;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/v1/me")
class StateController {
    private final StateService service;
    StateController(StateService service) { this.service = service; }

    @GetMapping
    Map<String, UUID> identity(@RequestAttribute UUID member) { return Map.of("memberId", member); }

    @PutMapping("/watchlist/{id}")
    @ResponseStatus(HttpStatus.NO_CONTENT)
    void add(@RequestAttribute UUID member, @PathVariable String id) { service.add(member, id); }

    @DeleteMapping("/watchlist/{id}")
    @ResponseStatus(HttpStatus.NO_CONTENT)
    void remove(@RequestAttribute UUID member, @PathVariable String id) { service.remove(member, id); }

    @GetMapping("/watchlist")
    Map<String, List<StateService.Saved>> saved(@RequestAttribute UUID member) {
        return Map.of("items", service.saved(member));
    }

    @PutMapping("/progress/{id}")
    StateService.Progress save(@RequestAttribute UUID member, @PathVariable String id, @RequestBody JsonNode body) {
        var position = body.path("positionMs");
        if (!position.isIntegralNumber() || !position.canConvertToLong()) throw StateService.invalidPosition();
        return service.save(member, id, position.longValue());
    }

    @GetMapping("/progress/{id}")
    StateService.Progress get(@RequestAttribute UUID member, @PathVariable String id) { return service.get(member, id); }

    @GetMapping("/continue-watching")
    Map<String, List<StateService.Continuing>> continuing(@RequestAttribute UUID member) {
        return Map.of("items", service.continuing(member));
    }
}
