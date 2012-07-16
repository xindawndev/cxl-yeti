#include <stdio.h>
#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

#include "YetiConfig.h"
#include "YetiDefs.h"
#include "YetiTypes.h"
#include "YetiDebug.h"

NAMEBEG

void yeti_debug_output(const char * message)
{
#if !defined(_WIN32_WCE)
    OutputDebugString(message);
#endif
    printf("%s", message);
}

NAMEEND
