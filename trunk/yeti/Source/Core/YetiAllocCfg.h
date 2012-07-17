#ifndef _CXL_YETI_ALLOCCFG_H_
#define _CXL_YETI_ALLOCCFG_H_

#include "YetiAllocObj.h"

#if defined(YETI_MEMORY_USING_NED_POOLING)
#   include "YetiNedPooling.h"
#elif defined(YETI_MEMORY_USING_NED_ALLOC)
#   include "YetiNedAlloc.h"
#else
// your allocators here?
#endif

NAMEBEG

enum MemoryCategory {
    MEMCATEGORY_GENERAL = 0
};

#if defined(YETI_MEMORY_USING_NED_POOLING)

#include "YetiNedPooling.h"

template <MemoryCategory cat>
class CategoriesedAllocPolicy : public NedAllocPolicy {};
template <MemoryCategory cat, YETI_UInt32 align = 0>
class CategorisedAlignAllocPolicy : public NedPoolingAlignedPolicy<align> {};

#elif defined(YETI_MEMORY_USING_NED_ALLOC)

#include "YetiNedAlloc.h"

template < MemoryCategory Cat >
class CategorisedAllocPolicy : public NedAllocPolicy {};
template < MemoryCategory Cat, YETI_UInt32 align = 0 >
class CategorisedAlignAllocPolicy : public NedAlignedAllocPolicy<align> {};

#else

// your allocators here?

#endif

typedef CategorisedAllocPolicy<MEMCATEGORY_GENERAL> GeneralAllocPolicy;

typedef AllocObj<GeneralAllocPolicy> GeneralAllocObj;

typedef GeneralAllocObj IObjAlloc;

NAMEEND



#endif // _CXL_YETI_ALLOCCFG_H_
