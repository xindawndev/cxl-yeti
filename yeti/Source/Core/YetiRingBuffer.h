#ifndef _CXL_YETI_RINGBUFFER_H_
#define _CXL_YETI_RINGBUFFER_H_

#include "YetiTypes.h"
#include "YetiReferences.h"

NAMEBEG

class RingBuffer
{
public:
    RingBuffer(YETI_Size size);
    RingBuffer(void * buffer, YETI_Size size);
    virtual ~RingBuffer();

    YETI_Size       get_space() const;
    YETI_Size       get_contiguous_space() const;
    YETI_Result     write(const void * buffer, YETI_Size byte_count);
    YETI_Size       get_available() const;
    YETI_Size       get_contiguous_available() const;
    YETI_Result     read(void * buffer, YETI_Size byte_count);
    unsigned char   read_byte();
    unsigned char   peek_byte(YETI_Position offset);
    YETI_Result     move_in(YETI_Position offset);
    YETI_Result     move_out(YETI_Position offset);
    YETI_Result     flush();
    YETI_Result     close();
    bool            is_closed() { return m_closed_; }

    unsigned char * get_write_pointer() { return m_in_; }
    unsigned char * get_read_pointer()  { return m_out_; }
private:
    struct {
        unsigned char * start;
        unsigned char * end;
    }               m_data_;
    unsigned char * m_in_;
    unsigned char * m_out_;
    YETI_Size       m_size_;
    bool            m_buffer_is_local_;
    bool            m_closed_;
};

typedef Reference<RingBuffer> RingBufferReference;

NAMEEND

#endif // _CXL_YETI_RINGBUFFER_H_
