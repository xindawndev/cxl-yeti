#ifndef _CXL_YETI_BUFFERED_STREAM_H_
#define _CXL_YETI_BUFFERED_STREAM_H_

#include "YetiStreams.h"
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiString.h"
#include "YetiDebug.h"

NAMEBEG

const YETI_Size YETI_BUFFERED_BYTE_STREAM_DEFAULT_SIZE = 4096;

class BufferedInputStream : public InputStream
{
public:
    BufferedInputStream(InputStreamReference & stream,
        YETI_Size buffer_size = YETI_BUFFERED_BYTE_STREAM_DEFAULT_SIZE);
    virtual ~BufferedInputStream();

    virtual YETI_Result read_line(String & line, YETI_Size max_chars = 4096, bool break_on_cr = false);
    virtual YETI_Result read_line(char * buffer, YETI_Size  buffer_size, YETI_Size * chars_read = NULL, bool break_on_cr = false);
    virtual YETI_Result set_buffer_size(YETI_Size size, bool force = false);
    virtual YETI_Result peek(void * buffer, YETI_Size  bytes_to_read, YETI_Size * bytes_read);

    YETI_Result read(void * buffer, YETI_Size  bytes_to_read, YETI_Size * bytes_read = NULL);
    YETI_Result seek(YETI_Position offset);
    YETI_Result tell(YETI_Position & offset);
    YETI_Result get_size(YETI_LargeSize & size);
    YETI_Result get_available(YETI_LargeSize & available);

protected:
    InputStreamReference m_source_;
    bool m_skipnewline_;
    bool m_eos_;
    struct {
        YETI_Byte * data_;
        YETI_Size offset_;
        YETI_Size valid_;
        YETI_Size size_;
    } m_buffer_;

    virtual YETI_Result fill_buffer();
    virtual YETI_Result release_buffer();
};

typedef Reference<BufferedInputStream> BufferedInputStreamReference;

NAMEEND

#endif // _CXL_YETI_BUFFERED_STREAM_H_
