#ifndef _CXL_YETI_BASE64_H_
#define _CXL_YETI_BASE64_H_

#include "YetiDataBuffer.h"
#include "YetiString.h"

NAMEBEG

const YETI_Cardinal NPT_BASE64_MIME_BLOCKS_PER_LINE = 19;
const YETI_Cardinal NPT_BASE64_PEM_BLOCKS_PER_LINE  = 16;

class Base64 {
public:
    static YETI_Result decode(const char* base64, 
        YETI_Size size,
        DataBuffer & data,
        bool url_safe = false);
    static YETI_Result encode(const YETI_Byte * data, 
        YETI_Size size, 
        String & base64, 
        YETI_Cardinal max_blocks_per_line = 0, 
        bool url_safe = false);

private: 
    Base64();
};

NAMEEND

#endif // _CXL_YETI_BASE64_H_
