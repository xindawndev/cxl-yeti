#include "YetiBase64.h"
#include "YetiUtil.h"
#include "YetiResults.h"

NAMEBEG

static const signed char Base64_Bytes[128] = {
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 0x3E,   -1,   -1,   -1, 0x3F,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,   -1,   -1,   -1, 0x7F,   -1,   -1,
    -1, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,   -1,   -1,   -1,   -1,   -1,
    -1, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33,   -1,   -1,   -1,   -1,   -1
};

static const char Base64_Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char YETI_BASE64_PAD_CHAR = '=';
const char YETI_BASE64_PAD_BYTE = 0x7F;

YETI_Result Base64::decode(const char * base64, 
                   YETI_Size size,
                   DataBuffer & data,
                   bool url_safe /* = false */)
{
    // estimate the data size
    data.set_buffer_size(size);

    // reset the buffer
    data.set_data_size(0);

    // keep a pointer to the buffer
    unsigned char* buffer = data.use_data();
    YETI_Size       data_size = 0;

    // iterate over all characters
    unsigned char codes[4];
    unsigned int code_count = 0;
    while (size--) {
        unsigned char c = *base64++;
        if (c >= YETI_ARRAY_SIZE(Base64_Bytes)) continue;
        if (url_safe) {
            // remap some characters
            if (c == '-') {
                c = '+';
            } else if (c == '_') {
                c = '/';
            }
        }
        signed char code = Base64_Bytes[c];
        if (code >= 0) {
            // valid code
            codes[code_count++] = code;
            if (code_count == 4) {
                // group complete
                if (codes[0] == YETI_BASE64_PAD_BYTE || codes[1] == YETI_BASE64_PAD_BYTE) {
                    return YETI_ERROR_INVALID_FORMAT;
                }
                if (codes[2] == YETI_BASE64_PAD_BYTE) {
                    // pad at char 3
                    if (codes[3] == YETI_BASE64_PAD_BYTE) {
                        // double padding
                        unsigned int packed = (codes[0]<<2)|(codes[1]>>4);
                        buffer[data_size++] = (unsigned char)packed;
                    } else {
                        // invalid padding
                        return YETI_ERROR_INVALID_FORMAT;
                    }
                } else if (codes[3] == YETI_BASE64_PAD_BYTE) {
                    // single padding
                    unsigned int packed = (codes[0]<<10)|(codes[1]<<4)|(codes[2]>>2);
                    buffer[data_size++] = (unsigned char)(packed >> 8);
                    buffer[data_size++] = (unsigned char)(packed     );
                } else {
                    // no padding
                    unsigned int packed = (codes[0]<<18)|(codes[1]<<12)|(codes[2]<<6)|codes[3];
                    buffer[data_size++] = (unsigned char)(packed >> 16);
                    buffer[data_size++] = (unsigned char)(packed >>  8);
                    buffer[data_size++] = (unsigned char)(packed      );
                }
                code_count = 0;
            }
        }
    }

    if (code_count) return YETI_ERROR_INVALID_FORMAT;

    // update the data size
    data.set_data_size(data_size);

    return YETI_SUCCESS;
}

YETI_Result Base64::encode(const YETI_Byte * data, 
                   YETI_Size size, 
                   String & base64, 
                   YETI_Cardinal max_blocks_per_line /* = 0 */, 
                   bool url_safe /* = false */)
{
    unsigned int block_count = 0;
    unsigned int           i = 0;

    // reserve space for the string
    base64.reserve(4*((size+3)/3) + 2*(max_blocks_per_line?(size/(3*max_blocks_per_line)):0));
    char* buffer = base64.use_chars();

    // encode each byte
    while (size >= 3) {
        // output a block
        *buffer++ = Base64_Chars[ (data[i  ] >> 2) & 0x3F];
        *buffer++ = Base64_Chars[((data[i  ] & 0x03) << 4) | ((data[i+1] >> 4) & 0x0F)];
        *buffer++ = Base64_Chars[((data[i+1] & 0x0F) << 2) | ((data[i+2] >> 6) & 0x03)];
        *buffer++ = Base64_Chars[  data[i+2] & 0x3F];

        size -= 3;
        i += 3;
        if (++block_count == max_blocks_per_line) {
            *buffer++ = '\r';
            *buffer++ = '\n';
            block_count = 0;
        }
    }

    // deal with the tail
    if (size == 2) {
        *buffer++ = Base64_Chars[ (data[i  ] >> 2) & 0x3F];
        *buffer++ = Base64_Chars[((data[i  ] & 0x03) << 4) | ((data[i+1] >> 4) & 0x0F)];
        *buffer++ = Base64_Chars[ (data[i+1] & 0x0F) << 2];
        *buffer++ = YETI_BASE64_PAD_CHAR;
    } else if (size == 1) {
        *buffer++ = Base64_Chars[(data[i] >> 2) & 0x3F];
        *buffer++ = Base64_Chars[(data[i] & 0x03) << 4];
        *buffer++ = YETI_BASE64_PAD_CHAR;
        *buffer++ = YETI_BASE64_PAD_CHAR;
    }

    // update the string size
    YETI_ASSERT((YETI_Size)(buffer-base64.get_chars()) <= base64.get_capacity());
    base64.set_length((YETI_Size)(buffer-base64.get_chars()));

    // deal with url safe remapping
    if (url_safe) {
        base64.replace('+','-');
        base64.replace('/','_');
    }

    return YETI_SUCCESS;
}

NAMEEND
