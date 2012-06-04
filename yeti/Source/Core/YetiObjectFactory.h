#ifndef _CXL_YETI_OBJECT_FACTORY_H_
#define _CXL_YETI_OBJECT_FACTORY_H_

#include "YetiMap.h"

NAMEBEG

template < class AbstractProduct, typename IdentifierType, typename ProductCreator >
class ObjectFactory
{
public:
    ObjectFactory() {
        m_associations_.clear();
    }

    ~ObjectFactory() {
        m_associations_.clear();
    }

    bool reg(const IdentifierType & id, ProductCreator creator) {
        return m_associations_.put(id, creator) == YETI_SUCCESS;
    }

    bool unreg(const IdentifierType & id) {
        return m_associations_.erase(id) == YETI_SUCCESS;
    }

    bool is_reg(const IdentifierType & id) {
        return m_associations_.has_key(id);
    }

    AbstractProduct * create_object(const IdentifierType & id) {
        ProductCreator * pc = NULL;
        YETI_Result i = m_associations_.get(id, pc);
        if (i == YETI_SUCCESS) {
            return (*pc)();
        }
        // handle error
    }
private:
    typedef Map<IdentifierType, AbstractProduct> AssocMap;
    AssocMap m_associations_;
};

NAMEEND

#endif // _CXL_YETI_OBJECT_FACTORY_H_
