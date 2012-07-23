#include "YetiNedPooling.h"

#if defined(YETI_MEMORY_USING_NED_POOLING)

#include <nedmalloc.c>

NAMEBEG

namespace _NedPoolingIntern
{
    const YETI_UInt32 s_pool_count = 14; // Needs to be greater than 4
    void * s_pool_footprint = reinterpret_cast<void *>(0xBB1AA45A);
    nedalloc::nedpool * s_pools[s_pool_count + 1] = { 0 };
    nedalloc::nedpool * s_pools_aligned[s_pool_count + 1] = { 0 };

    YETI_UInt32 pool_id_from_size(YETI_UInt32 a_reqSize)
    {
        // Requests size 16 or smaller are allocated at a 4 byte granularity.
        // Requests size 17 or larger are allocated at a 16 byte granularity.
        // With a s_poolCount of 14, requests size 177 or larger go in the default pool.

        // spreadsheet style =IF(B35<=16; FLOOR((B35-1)/4;1); MIN(FLOOR((B35-1)/16; 1) + 3; 14))

        YETI_UInt32 pool_id = 0;

        if (a_reqSize > 0) {
            if (a_reqSize <= 16) {
                pool_id = (a_reqSize - 1) >> 2;
            } else {
                //pool_id = std::min<YETI_UInt32>(((a_reqSize - 1) >> 4) + 3, s_pool_count);
            }
        }

        return pool_id;
    }

    void * internal_alloc(YETI_UInt32 a_reqSize)
    {
        YETI_UInt32 pool_id = pool_id_from_size(a_reqSize);
        nedalloc::nedpool* pool(0); // A pool pointer of 0 means the default pool.

        if (pool_id < s_pool_count) {
            if (s_pools[pool_id] == 0) {
                // Init pool if first use

                s_pools[pool_id] = nedalloc::nedcreatepool(0, 8);
                nedalloc::nedpsetvalue(s_pools[pool_id], s_pool_footprint); // All pools are stamped with a footprint
            }

            pool = s_pools[pool_id];
        }

        return nedalloc::nedpmalloc(pool, a_reqSize);
    }

    void* internal_alloc_aligned(YETI_UInt32 a_align, YETI_UInt32 a_req_size)
    {
        YETI_UInt32 poolID = pool_id_from_size(a_req_size);
        nedalloc::nedpool* pool(0); // A pool pointer of 0 means the default pool.

        if (poolID < s_pool_count) {
            if (s_pools_aligned[poolID] == 0) {
                // Init pool if first use

                s_pools_aligned[poolID] = nedalloc::nedcreatepool(0, 8);
                nedalloc::nedpsetvalue(s_pools_aligned[poolID], s_pool_footprint); // All pools are stamped with a footprint
            }

            pool = s_pools_aligned[poolID];
        }

        return nedalloc::nedpmemalign(pool, a_align, a_req_size);
    }

    void internalfree(void * a_mem)
    {
        if (a_mem) {
            nedalloc::nedpool* pool(0);

            void* footprint = nedalloc::nedgetvalue(&pool, a_mem);

            if (footprint == s_pool_footprint) {
                nedalloc::nedpfree(pool, a_mem);
            } else {
                // ...otherwise let nedalloc handle it.
                nedalloc::nedfree(a_mem);
            }
        }
    }
}

void* NedPoolingImpl::alloc_bytes(YETI_UInt32 count, 
                                 const char* file, int line, const char* func)
{
    void* ptr = _NedPoolingIntern::internal_alloc(count);
#if YETI_MEMORY_TRACKER
    MemoryTracker::get()._record_alloc(ptr, count, 0, file, line, func);
#else
    file = func = "";
    line = 0;
#endif
    return ptr;
}

void NedPoolingImpl::dealloc_bytes(void* ptr)
{
    if (!ptr)
        return;
#if YETI_MEMORY_TRACKER
    MemoryTracker::get()._record_dealloc(ptr);
#endif
    _NedPoolingIntern::internalfree(ptr);
}

void* NedPoolingImpl::alloc_bytes_aligned(YETI_UInt32 align, YETI_UInt32 count, 
                                        const char * file, int line, const char * func)
{
    void* ptr =  align ? _NedPoolingIntern::internal_alloc_aligned(align, count)
        : _NedPoolingIntern::internal_alloc_aligned(YETI_SIMD_ALIGNMENT, count);
#if YETI_MEMORY_TRACKER
    MemoryTracker::get()._record_alloc(ptr, count, 0, file, line, func);
#else
    // avoid unused params warning
    file = func = "";
    line = 0;
#endif
    return ptr;
}

void NedPoolingImpl::dealloc_bytes_aligned(YETI_UInt32 align, void * ptr)
{
    // deal with null
    if (!ptr)
        return;
#if YETI_MEMORY_TRACKER
    MemoryTracker::get()._record_dealloc(ptr);
#endif
    _NedPoolingIntern::internalfree(ptr);
}

NAMEEND

#endif
