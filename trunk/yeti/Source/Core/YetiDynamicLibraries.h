#ifndef _CXL_YETI_DYNAMIC_LIBRARIES_H_
#define _CXL_YETI_DYNAMIC_LIBRARIES_H_

#include "YetiTypes.h"

NAMEBEG

#define  YETI_DYNAMIC_LIBRARY_LOAD_FLAG_NOW 1

class DynamicLibraryInterface
{
public:
    virtual ~DynamicLibraryInterface() {}
    virtual YETI_Result find_symbol(const char * name, void *& symbol) = 0;
    virtual YETI_Result unload() = 0;
};

class DynamicLibrary : public DynamicLibraryInterface
{
public:
    static YETI_Result load(const char * name, YETI_Flags flags, DynamicLibrary *& library);
    ~DynamicLibrary() { delete m_delegate_; }

    virtual YETI_Result find_symbol(const char * name, void *& symbol) {
        return m_delegate_->find_symbol(name, symbol);
    }

    virtual YETI_Result unload() {
        return m_delegate_->unload();
    }

private:
    DynamicLibrary(DynamicLibraryInterface * delegate) : m_delegate_(delegate) {}
    DynamicLibraryInterface * m_delegate_;
};

NAMEEND

#endif // _CXL_YETI_DYNAMIC_LIBRARIES_H_
