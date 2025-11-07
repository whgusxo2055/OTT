# OTT Streaming Server Makefile

CC = clang
CFLAGS = -Wall -Wextra -std=c17 -O2 -pthread -g

# Homebrew paths (macOS)
BREW_PREFIX := $(shell brew --prefix)
SODIUM_PREFIX := $(shell brew --prefix libsodium)

INCLUDES = -Iinclude -Ithird_party/civetweb/include -Ithird_party/cJSON \
           -I$(SODIUM_PREFIX)/include
LDFLAGS = -L$(SODIUM_PREFIX)/lib
LIBS = -lsqlite3 -lsodium -lm

# Source files
SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:.c=.o)

# CivetWeb
CIVETWEB_DIR = third_party/civetweb
CIVETWEB_SRC = $(CIVETWEB_DIR)/src/civetweb.c
CIVETWEB_OBJ = $(CIVETWEB_SRC:.c=.o)
CIVETWEB_FLAGS = -DNO_SSL -DNO_CGI

# cJSON
CJSON_DIR = third_party/cJSON
CJSON_SRC = $(CJSON_DIR)/cJSON.c
CJSON_OBJ = $(CJSON_SRC:.c=.o)

# Target
TARGET = ott_server

# All objects
ALL_OBJ = $(OBJ) $(CIVETWEB_OBJ) $(CJSON_OBJ)

.PHONY: all clean run db-init db-reset deps test help

all: deps $(TARGET)

# Build main executable
$(TARGET): $(ALL_OBJ)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(ALL_OBJ) $(LIBS)
	@echo "Build complete: $(TARGET)"

# Compile C files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile CivetWeb with NO_SSL flag
$(CIVETWEB_OBJ): $(CIVETWEB_SRC)
	@echo "Compiling CivetWeb..."
	$(CC) $(CFLAGS) $(CIVETWEB_FLAGS) $(INCLUDES) -c $< -o $@

# Download and setup dependencies
deps:
	@echo "Setting up dependencies..."
	@mkdir -p third_party
	@if [ ! -d "$(CIVETWEB_DIR)" ]; then \
		echo "Downloading CivetWeb..."; \
		cd third_party && \
		git clone --depth 1 https://github.com/civetweb/civetweb.git; \
	fi
	@if [ ! -d "$(CJSON_DIR)" ]; then \
		echo "Downloading cJSON..."; \
		cd third_party && \
		git clone --depth 1 https://github.com/DaveGamble/cJSON.git; \
	fi
	@echo "Dependencies ready."

# Initialize database
db-init:
	@echo "Initializing database..."
	@sqlite3 app.db < migrations/V1__init.sql
	@echo "Database initialized: app.db"

# Reset database (WARNING: deletes all data)
db-reset:
	@echo "Resetting database..."
	@rm -f app.db
	@$(MAKE) db-init

# Run server
run: all
	@echo "Starting OTT server..."
	./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -f $(SRC_DIR)/*.o $(TARGET)
	@rm -f $(CIVETWEB_OBJ) $(CJSON_OBJ)
	@echo "Clean complete."

# Deep clean (includes dependencies)
clean-all: clean
	@echo "Deep cleaning..."
	@rm -rf third_party
	@rm -f app.db
	@echo "Deep clean complete."

# Run tests (placeholder)
test:
	@echo "Running tests..."
	@echo "Tests not yet implemented."

# Install system dependencies (macOS)
install-deps:
	@echo "Installing system dependencies..."
	@brew install sqlite3 libsodium ffmpeg
	@echo "System dependencies installed."

# Help
help:
	@echo "OTT Streaming Server - Makefile Help"
	@echo ""
	@echo "Available targets:"
	@echo "  make all          - Build the server (default)"
	@echo "  make deps         - Download third-party dependencies"
	@echo "  make db-init      - Initialize the database"
	@echo "  make db-reset     - Reset database (deletes all data)"
	@echo "  make run          - Build and run the server"
	@echo "  make clean        - Clean build artifacts"
	@echo "  make clean-all    - Clean everything including dependencies"
	@echo "  make test         - Run tests"
	@echo "  make install-deps - Install system dependencies (macOS)"
	@echo "  make help         - Show this help message"
