#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "uuid.h"

void uuid_generate(ott_uuid_t out) {
    // Generate random bytes
    unsigned char bytes[16];
    
    // Use /dev/urandom for better randomness
    FILE *f = fopen("/dev/urandom", "rb");
    if (f == NULL) {
        // Fallback to rand() if /dev/urandom is not available
        srand(time(NULL));
        for (int i = 0; i < 16; i++) {
            bytes[i] = rand() % 256;
        }
    } else {
        fread(bytes, 1, 16, f);
        fclose(f);
    }
    
    // Set version (4) and variant bits
    bytes[6] = (bytes[6] & 0x0F) | 0x40;  // Version 4
    bytes[8] = (bytes[8] & 0x3F) | 0x80;  // Variant 10
    
    // Format as UUID string: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    snprintf(out, 37,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5],
        bytes[6], bytes[7],
        bytes[8], bytes[9],
        bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]
    );
}
