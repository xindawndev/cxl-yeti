#include "YetiNedAlloc.h"

#include <nedmalloc.c>

NAMEBEG

void* NedAllocImpl::alloc_bytes(YETI_UInt32 count, const char * file, int line, const char * func)
{
    void * ptr = nedalloc::nedmalloc(count);
#ifdef YETI_MEMORY_TRACKER
    MemoryTracker::get()._record_alloc(ptr, count, 0, file, line, func);
#endif
    return ptr;
}

void NedAllocImpl::dealloc_bytes(void * ptr)
{
    if (!ptr)
        return;
#ifdef YETI_MEMORY_TRACKER
    MemoryTracker::get()._record_dealloc(ptr);
#endif
    nedalloc::nedfree(ptr);
}

void * NedAllocImpl::alloc_bytes_aligned(YETI_UInt32 align,
                                         YETI_UInt32 count, const char * file, int line, const char * func)
{
    // default to platform SIMD alignment if none specified
    void* ptr =  align ? nedalloc::nedmemalign(align, count)
        : nedalloc::nedmemalign(YETI_SIMD_ALIGNMENT, count);
#ifdef YETI_MEMORY_TRACKER
    MemoryTracker::get()._record_alloc(ptr, count, 0, file, line, func);
#endif
    return ptr;
}

void NedAllocImpl::dealloc_bytes_aligned(YETI_UInt32 align, void * ptr)
{
    if (!ptr)
        return;
#ifdef YETI_MEMORY_TRACKER
    MemoryTracker::get()._record_dealloc(ptr);
#endif
    nedalloc::nedfree(ptr);
}

NAMEEND
