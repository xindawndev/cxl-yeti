#include <stdarg.h>
#include <stdio.h>

#include "YetiConfig.h"
#include "YetiDefs.h"
#include "YetiTypes.h"
#include "YetiDebug.h"

NAMEBEG

void yeti_debug_output(const char* message)
{
    printf("%s", message);
}

NAMEEND
