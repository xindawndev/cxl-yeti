#ifndef _CXL_YETI_CONSOLE_H_
#define _CXL_YETI_CONSOLE_H_

#include "YetiTypes.h"
#include "YetiResults.h"

NAMEBEG

class Console
{
public:
    static void output(const char * message);
    static void output_f(const char * format, ...);
};

NAMEEND

#endif // _CXL_YETI_CONSOLE_H_
