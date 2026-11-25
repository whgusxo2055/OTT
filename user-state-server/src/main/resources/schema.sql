CREATE SCHEMA IF NOT EXISTS user_state;
CREATE TABLE IF NOT EXISTS user_state.member (
    id uuid PRIMARY KEY,
    display_name text NOT NULL,
    created_at timestamptz NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS user_state.watchlist (
    member_id uuid NOT NULL REFERENCES user_state.member(id),
    content_id text NOT NULL,
    added_at timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY (member_id, content_id)
);
CREATE TABLE IF NOT EXISTS user_state.watch_progress (
    member_id uuid NOT NULL REFERENCES user_state.member(id),
    content_id text NOT NULL,
    position_ms bigint NOT NULL CHECK (position_ms >= 0),
    updated_at timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY (member_id, content_id)
);
