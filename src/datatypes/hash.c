#include "hash.h"

static hash_t digest(char* ssn, uint32_t len) {
    uint32_t hash = 5381;
    for(int i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + (uint32_t)ssn[i];
    }
    return (hash_t) (hash % 256);
}

hash_t hash_ssn(char* ssn) {
    return digest(ssn, 12);
}
