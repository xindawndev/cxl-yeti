#include "YetiRingBuffer.h"
#include "YetiResults.h"
#include "YetiUtil.h"
#include "YetiStreams.h"

NAMEBEG

RingBuffer::RingBuffer(YETI_Size size)
: m_size_(size)
, m_buffer_is_local_(true)
, m_closed_(false)
{
    m_data_.start = new unsigned char[size];
    m_data_.end = m_data_.start + size;
    m_in_ = m_out_ = m_data_.start;
}

RingBuffer::RingBuffer(void * buffer, YETI_Size size)
: m_size_(size)
, m_buffer_is_local_(false)
, m_closed_(false)
{
    m_data_.start = (unsigned char *)buffer;
    m_data_.end = m_data_.start + size;
    m_in_ = m_out_ = m_data_.start;
}

RingBuffer::~RingBuffer()
{
    if (m_buffer_is_local_) delete [] m_data_.start;
}

YETI_Size RingBuffer::get_contiguous_space() const
{
    return
        (m_in_ < m_out_) ?
        (YETI_Size)(m_out_ - m_in_ - 1) :
    ((m_out_ == m_data_.start) ?
        (YETI_Size)(m_data_.end - m_in_ - 1) :
    (YETI_Size)(m_data_.end - m_in_));
}

YETI_Size RingBuffer::get_space() const
{
    return
        (m_in_ < m_out_) ?
        (YETI_Size)(m_out_ - m_in_ - 1) :
    (YETI_Size)(m_data_.end - m_in_ + m_out_ - m_data_.start - 1);
}

YETI_Size RingBuffer::get_contiguous_available() const
{
    return
        (m_out_ <= m_in_) ?
        (YETI_Size)(m_in_ - m_out_) :
    (YETI_Size)(m_data_.end - m_out_);
}

YETI_Size RingBuffer::get_available() const
{
    return 
        (m_out_ <= m_in_) ?
        (YETI_Size)(m_in_ - m_out_) :
    (YETI_Size)(m_data_.end - m_out_ + m_in_ - m_data_.start);
}

unsigned char RingBuffer::read_byte()
{
    unsigned char result = *m_out_++;
    if (m_out_ == m_data_.end) m_out_ = m_data_.start;
    return result;
}

unsigned char RingBuffer::peek_byte(YETI_Position offset)
{
    unsigned char * where;
    where = m_out_ + offset;
    if (where >= m_data_.end) where -= (m_data_.end - m_data_.start);
    return *where;
}

YETI_Result RingBuffer::move_in(YETI_Position offset)
{
    int fold;
    m_in_ += offset;
    fold = (int)(m_in_ - m_data_.end);
    if (fold >= 0) {
        m_in_ = m_data_.start + fold;
    }
    return YETI_SUCCESS;
}

YETI_Result RingBuffer::move_out(YETI_Position offset)
{
    int fold;
    m_out_ += offset;
    fold = (int)(m_out_ - m_data_.end);
    if (fold >= 0) {
        m_out_ = m_data_.start + fold;
    }
    return YETI_SUCCESS;
}

YETI_Result RingBuffer::flush()
{
    m_in_ = m_out_ = m_data_.start;
    return YETI_SUCCESS;
}

YETI_Result RingBuffer::close()
{
    m_closed_ = true;
    return YETI_SUCCESS;
}

YETI_Result RingBuffer::read(void * buffer, YETI_Size byte_count)
{
    if (m_closed_) return YETI_ERROR_READ_FAILED;
    if (byte_count == 0) return YETI_SUCCESS;
    if (m_in_ > m_out_) {
        if (buffer) MemoryCopy(buffer, m_out_, byte_count);
        m_out_ += byte_count;
        if (m_out_ == m_data_.end) m_out_ = m_data_.start;
    } else {
        unsigned int chunk = (unsigned int)(m_data_.end - m_out_);
        if (chunk >= byte_count) chunk = byte_count;

        if (buffer) MemoryCopy(buffer, m_out_, chunk);
        m_out_ += chunk;
        if (m_out_ == m_data_.end) m_out_ = m_data_.start;
        if (chunk != byte_count) {
            if (buffer) MemoryCopy(((char *)buffer) + chunk, m_out_, byte_count - chunk);
            m_out_ += byte_count - chunk;
            if (m_out_ == m_data_.end) m_out_ = m_data_.start;
        }
    }

    return YETI_SUCCESS;
}

YETI_Result RingBuffer::write(const void * buffer, YETI_Size byte_count)
{
    if (m_closed_) return YETI_ERROR_WRITE_FAILED;
    if (byte_count == 0) return YETI_SUCCESS;
    if (m_in_ < m_out_) {
        if (buffer) MemoryCopy(m_in_, buffer, byte_count);
        m_in_ += byte_count;
        if (m_in_ == m_data_.end) m_in_ = m_data_.start;
    } else {
        unsigned int chunk = (unsigned int)(m_data_.end - m_in_);
        if (chunk >= byte_count) chunk = byte_count;
        if (buffer) MemoryCopy(m_in_, buffer, chunk);
        m_in_ += chunk;
        if (m_in_ == m_data_.end) m_in_ = m_data_.start;
        if (chunk != byte_count) {
            if (buffer) MemoryCopy(m_in_, ((const char *)buffer) + chunk, byte_count - chunk);
            m_in_ += byte_count - chunk;
            if (m_in_ == m_data_.end) m_in_ = m_data_.start;
        }
    }

    return YETI_SUCCESS;
}

NAMEEND
