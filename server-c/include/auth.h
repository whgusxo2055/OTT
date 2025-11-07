#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include "types.h"

// Hash a password using libsodium
int auth_hash_password(const char *password, char *hash_out, size_t hash_len);

// Verify a password against a hash
bool auth_verify_password(const char *password, const char *hash);

// Parse HTTP Basic Auth header
int auth_parse_basic_header(const char *auth_header, char *username, char *password, size_t len);

// Authenticate user from Basic Auth header
int auth_authenticate_user(const char *auth_header, user_t *user);

#endif // AUTH_H
