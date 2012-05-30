#ifndef _CXL_YETI_INTERFACES_H_
#define _CXL_YETI_INTERFACES_H_

#include "YetiTypes.h"
#include "YetiCommon.h"
#include "YetiResults.h"
#include "YetiConfig.h"

const int YETI_ERROR_NO_SUCH_INTERFACE = YETI_ERROR_BASE_INTERFACES - 0;

NAMEBEG

class InterfaceId
{
public:
    bool operator ==(const InterfaceId & id) const {
        return ((id.m_id_ == m_id_) && (id.m_version_ == m_version_));
    }

    unsigned long m_id_;
    unsigned long m_version_;
};

class Polymorphic
{
public:
    virtual ~Polymorphic() {}

    virtual YETI_Result get_interface(const InterfaceId & id, YETI_Interface *& iface) = 0;
};

class Interruptible
{
public:
    virtual ~Interruptible() {}

    virtual YETI_Result interrupt() = 0;
};

class Configurable
{
public:
    virtual ~Configurable() {}

    virtual YETI_Result set_property(const char * /* name */
        , char * /* value */ ) {
        return YETI_ERROR_NO_SUCH_PROPERTY;
    }

    virtual YETI_Result set_property(const char * /* name */
        , int /* value */) {
        return YETI_ERROR_NO_SUCH_PROPERTY;
    }

    virtual YETI_Result set_property(const char * /* name */
        , PropertyValue & /* value */) {
        return YETI_ERROR_NO_SUCH_PROPERTY;
    }
};

NAMEEND

#endif // _CXL_YETI_INTERFACES_H_
