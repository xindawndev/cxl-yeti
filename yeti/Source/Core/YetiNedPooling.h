#ifndef _CXL_YETI_NETPOOLING_H_
#define _CXL_YETI_NETPOOLING_H_

#include "YetiTypes.h"
#include "YetiConstants.h"

NAMEBEG

class NedPoolingImpl
{
public:
    static void * alloc_bytes(YETI_UInt32 count, const char * file, int line, const char * func);
    static void dealloc_bytes(void * ptr);

    static void * alloc_bytes_aligned(YETI_UInt32 align, YETI_UInt32 count, const char * file, int line, const char * func);
    static void dealloc_bytes_aligned(YETI_UInt32 align, void * ptr);
};

class NedPoolingPolicy
{
public:
    static inline void * allocate_bytes(YETI_UInt32 count, const char * file = NULL, int line = 0, const char * func = NULL) {
        return NedPoolingImpl::alloc_bytes(count, file, line, func);
    }

    static inline void deallocate_bytes(void * ptr) {
        NedPoolingImpl::dealloc_bytes(ptr);
    }

    static inline YETI_UInt32 get_max_allocation_size() {
        return 0xFFFFFFFF;
    }

private:
    NedPoolingPolicy() {}
};

template < YETI_UInt32 Alignment = 0 >
class NedPoolingAlignedPolicy
{
    typedef int is_valid_alignment[Alignment <= 128 && ((Alignment & (Alignment - 1)) == 0) ? +1 : -1];

    static inline void * allocate_bytes(YETI_UInt32 count, const char * file = NULL, int line = 0, const char * func = NULL) {
        return NedPoolingImpl::alloc_bytes_aligned(Alignment, count, file, line, func);
    }

    static inline void deallocate_bytes(void * ptr) {
        NedPoolingImpl::dealloc_bytes_aligned(Alignment, ptr);
    }

    static inline YETI_UInt32 get_max_allocation_size() {
        return 0xFFFFFFFF;
    }

private:
    NedPoolingAlignedPolicy() {}

};

NAMEEND

#endif // _CXL_YETI_NETPOOLING_H_
