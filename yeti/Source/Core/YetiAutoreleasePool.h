#ifndef _CXL_YETI_AUTORELEASE_POOL_H_
#define _CXL_YETI_AUTORELEASE_POOL_H_

NAMEBEG

class AutoreleasePoolInterface
{
public:
    virtual ~AutoreleasePoolInterface() {}
};

class AutoreleasePool : public AutoreleasePoolInterface
{
public:
    AutoreleasePool();
    virtual ~AutoreleasePool() {}

protected:
    AutoreleasePoolInterface * m_delegate_;
};

NAMEEND

#endif // _CXL_YETI_AUTORELEASE_POOL_H_
