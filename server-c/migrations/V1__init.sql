PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;
PRAGMA temp_store = MEMORY;

-- Users table
CREATE TABLE IF NOT EXISTS users (
  id            TEXT PRIMARY KEY,
  login_id      TEXT NOT NULL UNIQUE,
  password_hash TEXT NOT NULL,
  display_name  TEXT NOT NULL,
  created_at    TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at    TEXT NOT NULL DEFAULT (datetime('now'))
);

-- Videos table
CREATE TABLE IF NOT EXISTS videos (
  id            TEXT PRIMARY KEY,
  title         TEXT NOT NULL,
  description   TEXT,
  duration_sec  INTEGER CHECK (duration_sec >= 0),
  mime_type     TEXT DEFAULT 'video/mp4',
  created_at    TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at    TEXT NOT NULL DEFAULT (datetime('now'))
);

-- Video files table
CREATE TABLE IF NOT EXISTS video_files (
  id           TEXT PRIMARY KEY,
  video_id     TEXT NOT NULL,
  file_path    TEXT NOT NULL,
  file_size    INTEGER CHECK (file_size >= 0),
  bitrate_kbps INTEGER,
  resolution   TEXT,
  created_at   TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (video_id) REFERENCES videos(id) ON DELETE CASCADE
);

-- Thumbnails table
CREATE TABLE IF NOT EXISTS thumbnails (
  id         TEXT PRIMARY KEY,
  video_id   TEXT NOT NULL,
  file_path  TEXT NOT NULL,
  width      INTEGER,
  height     INTEGER,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (video_id) REFERENCES videos(id) ON DELETE CASCADE
);

-- Watch history table
CREATE TABLE IF NOT EXISTS watch_history (
  id                TEXT PRIMARY KEY,
  user_id           TEXT NOT NULL,
  video_id          TEXT NOT NULL,
  last_position_sec INTEGER NOT NULL DEFAULT 0,
  completed         INTEGER NOT NULL DEFAULT 0,
  updated_at        TEXT NOT NULL DEFAULT (datetime('now')),
  UNIQUE (user_id, video_id),
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
  FOREIGN KEY (video_id) REFERENCES videos(id) ON DELETE CASCADE
);

-- Indexes
CREATE INDEX IF NOT EXISTS idx_videos_title ON videos(title);
CREATE INDEX IF NOT EXISTS idx_watch_history_user_video ON watch_history(user_id, video_id);
CREATE INDEX IF NOT EXISTS idx_video_files_video ON video_files(video_id);
CREATE INDEX IF NOT EXISTS idx_thumbnails_video ON thumbnails(video_id);
