#include <stdio.h>

#include "YetiConfig.h"
#include "YetiConsole.h"

NAMEBEG

void Console::output(const char* message)
{
    printf("%s", message);
}

NAMEEND
