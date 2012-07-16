#include <stdarg.h>
#include "YetiDebug.h"
#include "YetiUtil.h"

NAMEBEG

#define YETI_DEBUG_LOCAL_BUFFER_SIZE 1024
#define YETI_DEBUG_BUFFER_INCREMENT  4096
#define YETI_DEBUG_BUFFER_MAX_SIZE   65536

void yeti_debug(const char * format, ...)
{
#if defined(YETI_DEBUG)
    char local_buffer[YETI_DEBUG_LOCAL_BUFFER_SIZE];
    unsigned int buffer_size = YETI_DEBUG_LOCAL_BUFFER_SIZE;
    char * buffer = local_buffer;
    va_list args;
    
    va_start(args, format);
    for (;;) {
        int result;
        result = FormatStringVN(buffer, buffer_size - 1, format, args);
        if (result >= 0) break;

        buffer_size = (buffer_size + YETI_DEBUG_BUFFER_INCREMENT) * 2;
        if (buffer_size > YETI_DEBUG_BUFFER_MAX_SIZE) break;
        if (buffer != local_buffer) delete []buffer;
        buffer = new char[buffer_size];
        if (buffer == NULL) return;
    }

    yeti_debug_output(buffer);
    if (buffer != local_buffer) delete []buffer;
    va_end(args);
#else
    YETI_COMPILER_UNUSED(format);
#endif
}

NAMEEND
