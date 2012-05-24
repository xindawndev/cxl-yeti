#ifndef _CXL_YETI_HASH_H_
#define _CXL_YETI_HASH_H_

#include "YetiTypes.h"
#include "YetiResults.h"

NAMEBEG

const YETI_UInt32 FNV1A_32_INIT = ((YETI_UInt32)0x811c9dc5);
YETI_UInt32 fnv1a_hash32(const YETI_UInt8 * data, YETI_Size data_size, YETI_UInt32 hash_init = FNV1A_32_INIT);
YETI_UInt32 fnv1a_hashstr32(const char * data, YETI_UInt32 hash_init = FNV1A_32_INIT);

const YETI_UInt64 FNV1A_64_INIT = ((YETI_UInt64)0xcbf29ce484222325ULL);
YETI_UInt64 fnv1a_hash64(const YETI_UInt8 * data, YETI_Size data_size, YETI_UInt64 hash_init = FNV1A_64_INIT);
YETI_UInt64 fnv1a_hashstr64(const char * data, YETI_UInt64 hash_init = FNV1A_64_INIT);

template < typename K >
struct Hash
{
};

template <>
struct Hash<const char *>
{
    YETI_UInt32 operator()(const char * s) const { return fnv1a_hashstr32(s); }
};

template <>
struct Hash<char *>
{
    YETI_UInt32 operator()(char * s) const { return fnv1a_hashstr32(s); }
};

template <>
struct Hash<int>
{
    YETI_UInt32 operator()(int i) const { return fnv1a_hash32(reinterpret_cast<const YETI_UInt8 *>(&i), sizeof(int)); }
};

template <>
struct Hash<unsigned int>
{
    YETI_UInt32 operator()(unsigned int i) const { return fnv1a_hash32(reinterpret_cast<const YETI_UInt8 *>(&i), sizeof(int)); }
};

NAMEEND

#endif // _CXL_YETI_HASH_H_
