#include "YetiTypes.h"
#include "YetiResults.h"
#include "YetiHash.h"

NAMEBEG

const YETI_UInt32 FNV_32_PRIME = 0x01000193;

YETI_UInt32 fnv1a_hash32(const YETI_UInt8 * data, YETI_Size data_size, YETI_UInt32 hash_init)
{
    const YETI_UInt8 *  data_end = data + data_size;
    YETI_UInt32         hash_value = hash_init;

    while (data < data_end) {
        hash_value ^= (YETI_UInt32)*data++;
#if defined(YETI_CONFIG_FNV_HASH_USE_SHIFT_MUL)
        hash_value += (hash_value << 1) + (hash_value << 4) + (hash_value << 7) + (hash_value << 8) + (hash_value << 24);
#else
        hash_value *= FNV_32_PRIME;
#endif
    }

    return hash_value;
}

YETI_UInt32 fnv1a_hashstr32(const char * data, YETI_UInt32 hash_init)
{
    YETI_UInt32 hash_value = hash_init;

    while (*data) {
        hash_value ^= (YETI_UInt32)*data++;
#if defined(YETI_CONFIG_FNV_HASH_USE_SHIFT_MUL)
        hash_value += (hash_value << 1) + (hash_value << 4) + (hash_value << 7) + (hash_value << 8) + (hash_value << 24);
#else
        hash_value *= FNV_32_PRIME;
#endif
    }

    return hash_value;
}

const YETI_UInt64 FNV_64_PRIME = 0x100000001b3ULL;
YETI_UInt64 fnv1a_hash64(const YETI_UInt8 * data, YETI_Size data_size, YETI_UInt64 hash_init)
{
    const YETI_UInt8 * data_end = data + data_size;
    YETI_UInt64 hash_value = hash_init;

    while (data < data_end) {
        hash_value ^= (YETI_UInt64)*data++;
#if defined(YETI_CONFIG_FNV_HASH_USE_SHIFT_MUL)
        hash_value += (hash_value << 1) + (hash_value << 4) + (hash_value << 5) + (hash_value << 7) + (hash_value << 8) + (hash_value << 40);
#else
        hash_value *= FNV_64_PRIME;
#endif
    }

    return hash_value;
}

YETI_UInt64 fnv1a_hashstr64(const char * data, YETI_UInt64 hash_init)
{
    YETI_UInt64 hash_value = hash_init;

    while (*data) {
        hash_value ^= (YETI_UInt64)*data++;
#if defined(YETI_CONFIG_FNV_HASH_USE_SHIFT_MUL)
        hash_value += (hash_value << 1) + (hash_value << 4) + (hash_value << 5) + (hash_value << 7) + (hash_value << 8) + (hash_value << 40);
#else
        hash_value *= FNV_64_PRIME;
#endif
    }

    return hash_value;
}

NAMEEND
