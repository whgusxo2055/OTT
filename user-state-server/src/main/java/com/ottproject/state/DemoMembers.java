package com.ottproject.state;

import java.util.Map;
import java.util.UUID;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Profile;
import org.springframework.jdbc.core.simple.JdbcClient;
import org.springframework.stereotype.Component;

@Component
@Profile("demo")
class DemoMembers implements MemberResolver {
    private final Map<String, UUID> members;

    DemoMembers(JdbcClient db, @Value("${DEMO_TOKEN_A}") String a, @Value("${DEMO_TOKEN_B}") String b) {
        if (a.isBlank() || b.isBlank() || a.equals(b)) {
            throw new IllegalArgumentException("Two distinct demo tokens are required");
        }
        members = Map.of("Bearer " + a, seed(db, 1, "회원 A"), "Bearer " + b, seed(db, 2, "회원 B"));
    }

    private UUID seed(JdbcClient db, int number, String name) {
        UUID id = new UUID(0, number);
        db.sql("INSERT INTO user_state.member(id, display_name) VALUES (?, ?) ON CONFLICT DO NOTHING")
                .params(id, name).update();
        return id;
    }

    public UUID resolve(String authorization) {
        UUID member = authorization == null ? null : members.get(authorization);
        if (member == null) throw new StateFailure(401, "UNAUTHENTICATED", "회원 인증이 필요합니다.");
        return member;
    }
}
