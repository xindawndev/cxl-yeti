#include "YetiString.h"

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiResults.h"
#include "YetiUtil.h"
#include "YetiDebug.h"

NAMEBEG

#define YETI_STRINGS_WHITESPACE_CHARS "\r\n\t"

const unsigned int YETI_STRING_FORMAT_BUFFER_DEFAULT_SIZE = 256;
const unsigned int YETI_STRING_FORMAT_BUFFER_MAX_SIZE     = 0x80000; // 512k

inline char _uppercase(char x) {
    return (x >= 'a' && x <= 'z') ? x&0xdf : x;
}

inline char _lowercase(char x) {
    return (x >= 'A' && x <= 'Z') ? x^32 : x;
}

char String::empty_string_ = '\0';

String String::from_integer(YETI_Int64 value)
{
    char str[32];
    char * c = &str[31];

    return String(c);
}

NAMEEND
