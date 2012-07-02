#ifndef _CXL_YETI_WIN32_NETWORK_H_
#define _CXL_YETI_WIN32_NETWORK_H_

NAMEBEG

class WinsockSystem
{
public:
    static WinsockSystem Initializer;
    ~WinsockSystem();

private:
    WinsockSystem();
};

NAMEEND

#endif // _CXL_YETI_WIN32_NETWORK_H_
