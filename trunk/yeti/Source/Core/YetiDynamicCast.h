#ifndef _CXL_YETI_DYNAMICCAST_H_
#define _CXL_YETI_DYNAMICCAST_H_

#include "YetiTypes.h"
#include "YetiCommon.h"
#include "YetiResults.h"
#include "YetiConfig.h"

#if defined(YETI_CONFIG_NO_RTTI)

#define YETI_DYNAMIC_CAST(_class, _object) \
    ( ((_object)==0) ? 0 : reinterpret_cast<_class*>((_object)->DynamicCast(&_class::_class_##_class)) )
#define YETI_IMPLEMENT_DYNAMIC_CAST(_class)                 \
    static int _class_##_class;                             \
    virtual void* DynamicCast(const void* class_anchor) {   \
    if (class_anchor ==  &_class::_class_##_class) {        \
    return static_cast<_class*>(this);                      \
    }                                                       \
    return NULL;                                            \
}
#define YETI_IMPLEMENT_DYNAMIC_CAST_D(_class, _supercalss)\
    static int _class_##_class;                           \
    virtual void* DynamicCast(const void* class_anchor) { \
    if (class_anchor ==  &_class::_class_##_class) {      \
    return static_cast<_class*>(this);                    \
    } else {                                              \
    return _superclass::DynamicCast(class_anchor);        \
    }                                                     \
}
#define YETI_IMPLEMENT_DYNAMIC_CAST_D2(_class, _superclass, _mixin)\
    static int _class_##_class;                                    \
    virtual void* DynamicCast(const void* class_anchor) {          \
    if (class_anchor ==  &_class::_class_##_class) {               \
    return static_cast<_class*>(this);                             \
    } else {                                                       \
    void* sup = _superclass::DynamicCast(class_anchor);            \
    if (sup) return sup;                                           \
    return _mixin::DynamicCast(class_anchor);                      \
    }                                                              \
}
#define YETI_DEFINE_DYNAMIC_CAST_ANCHOR(_class) int _class::_class_##_class = 0;

#else

#define YETI_DYNAMIC_CAST(_class, _object) dynamic_cast< _class * >(_object)
#define YETI_IMPLEMENT_DYNAMIC_CAST(_class)
#define YETI_IMPLEMENT_DYNAMIC_CAST_D(_class, _supercalss)
#define YETI_IMPLEMENT_DYNAMIC_CAST_D2(_class, _superclass, _mixin)
#define YETI_DEFINE_DYNAMIC_CAST_ANCHOR(_class)

#endif

#endif // _CXL_YETI_DYNAMICCAST_H_
