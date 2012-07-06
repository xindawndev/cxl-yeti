#ifndef _CXL_YETI_NONCOPYABLE_H_
#define _CXL_YETI_NONCOPYABLE_H_

NAMEBEG

class Noncopyable
{
public:
    Noncopyable() {}
    ~Noncopyable() {}
private:
    Noncopyable(const Noncopyable &);
    const Noncopyable & operator=(const Noncopyable &);
};

NAMEEND

#endif // _CXL_YETI_NONCOPYABLE_H_
