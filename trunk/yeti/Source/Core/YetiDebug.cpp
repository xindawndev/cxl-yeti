#include <stdarg.h>
#include "YetiDebug.h"

NAMEBEG

#define YETI_DEBUG_LOCAL_BUFFER_SIZE 1024
#define YETI_DEBUG_BUFFER_INCREMENT  4096
#define YETI_DEBUG_BUFFER_MAX_SIZE   65536

void yeti_debug(const char * format, ...)
{
#if defined(YETI_DEBUG)
#else
    YETI_COMPILER_UNUSED(format);
#endif
}

NAMEEND
