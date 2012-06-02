#include "YetiUtil.h"

#include <math.h>
#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiDebug.h"
#include "YetiResults.h"

#if defined(YETI_CONFIG_HAVE_LIMITS_H)
#   include <limits.h>
#endif

NAMEBEG

const unsigned int YETI_FORMAT_LOCAL_BUFFER_SIZE = 1024;
const unsigned int YETI_FORMAT_BUFFER_INCREMENT  = 4096;
const unsigned int YETI_FORMAT_BUFFER_MAX_SIZE   = 65536;

YETI_UInt64 bytes_to_int64_be(const unsigned char * buffer)
{
    return 
        (((YETI_UInt64)buffer[0]) << 56) |
        (((YETI_UInt64)buffer[1]) << 48) |
        (((YETI_UInt64)buffer[2]) << 40) |
        (((YETI_UInt64)buffer[3]) << 32) |
        (((YETI_UInt64)buffer[4]) << 24) |
        (((YETI_UInt64)buffer[5]) << 16) |
        (((YETI_UInt64)buffer[6]) << 8)  |
        (((YETI_UInt64)buffer[7])        );
}

YETI_UInt32 bytes_to_int32_be(const unsigned char * buffer)
{
    return 
        (((YETI_UInt32)buffer[0]) << 24) |
        (((YETI_UInt32)buffer[1]) << 16) |
        (((YETI_UInt32)buffer[2]) << 8  )|
        (((YETI_UInt32)buffer[3])       );
}

YETI_UInt32 bytes_to_int24_be(const unsigned char * buffer)
{
    return 
        (((YETI_UInt32)buffer[0]) << 16) |
        (((YETI_UInt32)buffer[1]) << 8  )|
        (((YETI_UInt32)buffer[2])       );
}

YETI_UInt32 bytes_to_int16_be(const unsigned char * buffer)
{
    return 
        (((YETI_UInt32)buffer[0]) << 8  )|
        (((YETI_UInt32)buffer[1])       );
}

YETI_UInt64 bytes_to_int64_le(const unsigned char * buffer)
{
    return 
        (((YETI_UInt64)buffer[7]) << 56) |
        (((YETI_UInt64)buffer[6]) << 48) |
        (((YETI_UInt64)buffer[5]) << 40) |
        (((YETI_UInt64)buffer[4]) << 32) |
        (((YETI_UInt64)buffer[3]) << 24) |
        (((YETI_UInt64)buffer[2]) << 16) |
        (((YETI_UInt64)buffer[1]) << 8)  |
        (((YETI_UInt64)buffer[0])        );
}

YETI_UInt32 bytes_to_int32_le(const unsigned char * buffer)
{
    return 
        (((YETI_UInt32)buffer[3]) << 24) |
        (((YETI_UInt32)buffer[2]) << 16) |
        (((YETI_UInt32)buffer[1]) << 8  )|
        (((YETI_UInt32)buffer[0])       );
}

YETI_UInt32 bytes_to_int24_le(const unsigned char * buffer)
{
    return 
        (((YETI_UInt32)buffer[2]) << 16) |
        (((YETI_UInt32)buffer[1]) << 8  )|
        (((YETI_UInt32)buffer[0])       );
}

YETI_UInt32 bytes_to_int16_le(const unsigned char * buffer)
{
    return 
        (((YETI_UInt32)buffer[1]) << 8  )|
        (((YETI_UInt32)buffer[0])       );
}

void bytes_from_int64_be(unsigned char * buffer, YETI_UInt64 value)
{
    buffer[0] = (unsigned char)(value >> 56) & 0xFF;
    buffer[1] = (unsigned char)(value >> 48) & 0xFF;
    buffer[2] = (unsigned char)(value >> 40) & 0xFF;
    buffer[3] = (unsigned char)(value >> 32) & 0xFF;
    buffer[4] = (unsigned char)(value >> 24) & 0xFF;
    buffer[5] = (unsigned char)(value >> 16) & 0xFF;
    buffer[6] = (unsigned char)(value >>  8) & 0xFF;
    buffer[7] = (unsigned char)(value      ) & 0xFF;
}

void bytes_from_int32_be(unsigned char * buffer, YETI_UInt64 value)
{
    buffer[0] = (unsigned char)(value >> 24) & 0xFF;
    buffer[1] = (unsigned char)(value >> 16) & 0xFF;
    buffer[2] = (unsigned char)(value >>  8) & 0xFF;
    buffer[3] = (unsigned char)(value      ) & 0xFF;
}

void bytes_from_int24_be(unsigned char * buffer, YETI_UInt64 value)
{
    buffer[0] = (unsigned char)(value >> 16) & 0xFF;
    buffer[1] = (unsigned char)(value >>  8) & 0xFF;
    buffer[2] = (unsigned char)(value      ) & 0xFF;
}

void bytes_from_int16_be(unsigned char * buffer, YETI_UInt64 value)
{
    buffer[0] = (unsigned char)(value >>  8) & 0xFF;
    buffer[1] = (unsigned char)(value      ) & 0xFF;
}

void bytes_from_int64_le(unsigned char * buffer, YETI_UInt64 value)
{
    buffer[7] = (unsigned char)(value >> 56) & 0xFF;
    buffer[6] = (unsigned char)(value >> 48) & 0xFF;
    buffer[5] = (unsigned char)(value >> 40) & 0xFF;
    buffer[4] = (unsigned char)(value >> 32) & 0xFF;
    buffer[3] = (unsigned char)(value >> 24) & 0xFF;
    buffer[2] = (unsigned char)(value >> 16) & 0xFF;
    buffer[1] = (unsigned char)(value >>  8) & 0xFF;
    buffer[0] = (unsigned char)(value      ) & 0xFF;
}

void bytes_from_int32_le(unsigned char * buffer, YETI_UInt64 value)
{
    buffer[3] = (unsigned char)(value >> 24) & 0xFF;
    buffer[2] = (unsigned char)(value >> 16) & 0xFF;
    buffer[1] = (unsigned char)(value >>  8) & 0xFF;
    buffer[0] = (unsigned char)(value      ) & 0xFF;
}

void bytes_from_int24_le(unsigned char * buffer, YETI_UInt64 value)
{
    buffer[2] = (unsigned char)(value >> 16) & 0xFF;
    buffer[1] = (unsigned char)(value >>  8) & 0xFF;
    buffer[0] = (unsigned char)(value      ) & 0xFF;
}

void bytes_from_int16_le(unsigned char * buffer, YETI_UInt64 value)
{
    buffer[1] = (unsigned char)(value >>  8) & 0xFF;
    buffer[0] = (unsigned char)(value      ) & 0xFF;
}

#if !defined(YETI_CONFIG_HAVE_SNPRINTF)
int FormatString(char * /*str*/, YETI_Size /*size*/, const char * /*format*/, ...)
{
    YETI_ASSERT(0);
    return 0;
}
#endif

char nibble_to_hex(unsigned int nibble, bool uppercase /* = true */)
{
    YETI_ASSERT(nibble < 16);
    if (uppercase) {
        return (nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10));
    } else {
        return (nibble < 10) ? ('0' + nibble) : ('a' + (nibble - 10));
    }
}

int hex_to_nibble(char hex)
{
    if (hex >= 'a' && hex <= 'f') {
        return ((hex - 'a') + 10);
    } else if (hex >= 'A' && hex <= 'F') {
        return ((hex - 'A') + 10);
    } else if (hex >= '0' && hex <= '9') {
        return (hex - '0');
    }

    return -1;
}

void byte_to_hex(YETI_Byte b, char * buffer, bool uppercase /* = false */)
{
    buffer[0] = nibble_to_hex((b >> 4) & 0x0F, uppercase);
    buffer[1] = nibble_to_hex(b        & 0x0F, uppercase);
}

YETI_Result hex_to_byte(const char * buffer, YETI_Byte & b)
{
    int nibble_0 = hex_to_nibble(buffer[0]);
    if (nibble_0 < 0) return YETI_ERROR_INVALID_SYNTAX;
    int nibble_1 = hex_to_nibble(buffer[1]);
    if (nibble_1 < 0) return YETI_ERROR_INVALID_SYNTAX;

    b = (nibble_0 << 4) | nibble_1;
    return YETI_SUCCESS;
}

YETI_Result hex_to_bytes(const char * buffer, DataBuffer & bytes)
{
    YETI_Size len = StringLength(buffer);
    if ((len % 2) != 0) return YETI_ERROR_INVALID_PARAMETERS;
    YETI_Size bytes_size = len / 2;
    YETI_Result result = bytes.set_buffer_size(bytes_size);
    if (YETI_FAILED(result)) return result;

    for (YETI_Ordinal i = 0; i < bytes_size; ++i) {
        result = hex_to_byte(buffer + (i * 2), *(bytes.use_data() + i));
        if (YETI_FAILED(result)) return result;
    }

    return YETI_SUCCESS;
}

String hex_string(const unsigned char * data, YETI_Size data_size, const char * separator /* = NULL */, bool uppercase /* = false */)
{
    String result;
    if (data == NULL || data_size == 0) return result;
    return result;
}

NAMEEND
