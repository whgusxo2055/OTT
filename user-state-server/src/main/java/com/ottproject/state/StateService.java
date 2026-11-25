package com.ottproject.state;

import java.time.OffsetDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.springframework.jdbc.core.simple.JdbcClient;
import org.springframework.stereotype.Service;

@Service
class StateService {
    record Saved(String contentId, String title, OffsetDateTime addedAt) {}
    record WatchRow(String contentId, OffsetDateTime addedAt) {}
    record Position(String contentId, long positionMs, OffsetDateTime updatedAt) {}
    record Progress(String contentId, long positionMs, long durationMs, boolean completed,
                    Long resumePositionMs, OffsetDateTime updatedAt) {}
    record Continuing(String contentId, String title, long durationMs, long resumePositionMs, OffsetDateTime updatedAt) {}

    private final JdbcClient db;
    private final Catalog catalog;

    StateService(JdbcClient db, Catalog catalog) {
        this.db = db;
        this.catalog = catalog;
    }

    void add(UUID member, String id) {
        catalog.title(id);
        db.sql("INSERT INTO user_state.watchlist(member_id, content_id) VALUES (?, ?) ON CONFLICT DO NOTHING")
                .params(member, id).update();
    }

    void remove(UUID member, String id) {
        db.sql("DELETE FROM user_state.watchlist WHERE member_id=? AND content_id=?").params(member, id).update();
    }

    List<Saved> saved(UUID member) {
        var rows = db.sql("""
                SELECT content_id, added_at FROM user_state.watchlist WHERE member_id=?
                ORDER BY added_at DESC, content_id
                """).param(member).query(WatchRow.class).list();
        var items = new ArrayList<Saved>();
        for (var row : rows) {
            try {
                items.add(new Saved(row.contentId(), catalog.title(row.contentId()), row.addedAt()));
            } catch (StateFailure e) {
                if (e.status != 404) throw e;
            }
        }
        return items;
    }

    Progress save(UUID member, String id, long position) {
        long duration = catalog.duration(id);
        if (position < 0 || position > duration) throw invalidPosition();
        var row = db.sql("""
                INSERT INTO user_state.watch_progress AS p(member_id, content_id, position_ms) VALUES (?, ?, ?)
                ON CONFLICT (member_id, content_id) DO UPDATE SET position_ms=excluded.position_ms,
                    updated_at=CASE WHEN p.position_ms=excluded.position_ms THEN p.updated_at ELSE clock_timestamp() END
                RETURNING content_id, position_ms, updated_at
                """).params(member, id, position).query(Position.class).single();
        return progress(row, duration);
    }

    Progress get(UUID member, String id) {
        long duration = catalog.duration(id);
        var row = db.sql("SELECT content_id, position_ms, updated_at FROM user_state.watch_progress WHERE member_id=? AND content_id=?")
                .params(member, id).query(Position.class).optional().orElse(new Position(id, 0, null));
        return progress(row, duration);
    }

    List<Continuing> continuing(UUID member) {
        var rows = db.sql("""
                SELECT content_id, position_ms, updated_at FROM user_state.watch_progress
                WHERE member_id=? AND position_ms >= 30000 ORDER BY updated_at DESC, content_id
                """).param(member).query(Position.class).list();
        var items = new ArrayList<Continuing>();
        for (var row : rows) {
            try {
                var state = progress(row, catalog.duration(row.contentId()));
                if (state.resumePositionMs() != null) {
                    items.add(new Continuing(row.contentId(), catalog.title(row.contentId()), state.durationMs(),
                            state.resumePositionMs(), row.updatedAt()));
                }
            } catch (StateFailure e) {
                if (e.status != 404 && e.status != 409) throw e;
            }
        }
        return items;
    }

    private Progress progress(Position row, long duration) {
        boolean completed = row.positionMs() >= duration - duration / 10;
        Long resume = !completed && row.positionMs() > 0 ? row.positionMs() : null;
        return new Progress(row.contentId(), row.positionMs(), duration, completed, resume, row.updatedAt());
    }

    static StateFailure invalidPosition() {
        return new StateFailure(400, "INVALID_POSITION", "시청 위치는 영상 길이 이내의 밀리초 정수여야 합니다.");
    }
}
