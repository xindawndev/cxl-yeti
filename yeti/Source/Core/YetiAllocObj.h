#ifndef _CXL_YETI_ALLOCOBJ_H_
#define _CXL_YETI_ALLOCOBJ_H_

#include "YetiTypes.h"

#ifdef new
# undef new
#endif

#ifdef delete
# undef delete
#endif

NAMEBEG

template< class Alloc >
class AllocObj
{
public:
    explicit AllocObj() {}
    ~AllocObj() {}

    void * operator new(YETI_UInt32 sz, const char * file, int line, const char * func) {
        return Alloc::allocate_bytes(sz, file, line, func);
    }

    void * operator new(YETI_UInt32 sz) {
        return Alloc::allocate_bytes(sz);
    }

    void * operator new(YETI_UInt32 sz, void * ptr) {
        (void) sz;
        return ptr;
    }

    void * operator new[] (YETI_UInt32 sz, const char * file, int line, const char * func) {
        return Alloc::allocate_bytes(sz, file, line, func);
    }

    void * operator new[] (YETI_UInt32 sz) {
        return Alloc::allocate_bytes(sz);
    }

    void operator delete(void * ptr) {
        Alloc::deallocate_bytes(ptr);
    }

    void operator delete(void * ptr, void *) {
        Alloc::deallocate_bytes(ptr);
    }

    void operator delete(void * ptr, const char *, int, const char *) {
        Alloc::deallocate_bytes(ptr);
    }

    void operator delete[] (void * ptr) {
        Alloc::deallocate_bytes(ptr);
    }

    void operator delete[] (void * ptr, const char *, int, const char *) {
        Alloc::deallocate_bytes(ptr);
    }
};

NAMEEND

#endif // _CXL_YETI_ALLOCOBJ_H_
