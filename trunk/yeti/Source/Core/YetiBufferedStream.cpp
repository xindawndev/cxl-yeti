#include "YetiBufferedStream.h"

#include "YetiTypes.h"
#include "YetiInterfaces.h"
#include "YetiConstants.h"
#include "YetiUtil.h"

#define YETI_CHECK_NOLOGTIMEOUT(_x)     \
    do {                                \
        YETI_Result __result = (_x);    \
        if (__result != YETI_SUCCESS) { \
            if (__result != YETI_ERROR_TIMEOUT && __result != YETI_ERROR_EOS) { \
                YETI_CHECK(__result);   \
            }                           \
            return __result;            \
        }                               \
    } while(0)

NAMEBEG

BufferedInputStream::BufferedInputStream(InputStreamReference & stream,
                                         YETI_Size buffer_size /*= YETI_BUFFERED_BYTE_STREAM_DEFAULT_SIZE*/)
                                         : m_source_(stream)
                                         , m_skipnewline_(false)
                                         , m_eos_(false)
{
    m_buffer_.data_ = NULL;
    m_buffer_.offset_ = 0;
    m_buffer_.valid_ = 0;
    m_buffer_.size_ = buffer_size;
}

BufferedInputStream::~BufferedInputStream()
{
    delete []m_buffer_.data_;
}

YETI_Result BufferedInputStream::read_line(String & line, YETI_Size max_chars/* = 4096*/, bool break_on_cr/* = false*/);
YETI_Result BufferedInputStream::read_line(char * buffer, YETI_Size  buffer_size, YETI_Size * chars_read/* = NUL*/L, bool break_on_cr/* = false*/);
YETI_Result BufferedInputStream::set_buffer_size(YETI_Size size, bool force/* = false*/)
{
    if (m_buffer_.DATA_BLOB != NULL) {
        if (m_buffer_.size_ < size || force) {
            YETI_Byte * buffer = new YETI_Byte[size];
            if (buffer == NULL) return YETI_ERROR_OUT_OF_MEMORY;

            YETI_Size need_to_copy = m_buffer_.valid_ - m_buffer_.offset_;
            if (need_to_copy) {
                CopyMemory((void *)buffer, m_buffer_.data_ + m_buffer_.offset_, need_to_copy);
            }

            delete []m_buffer_.data_;
            m_buffer_.data_ = buffer;
            m_buffer_.valid_ -= m_buffer_.offset_;
            m_buffer_.offset_ = 0;
        }
    }
    m_buffer_.size_ = size;
    return YETI_SUCCESS
}

YETI_Result BufferedInputStream::fill_buffer()
{
    if (m_eos_) return YETI_ERROR_EOS;

    YETI_ASSERT(m_buffer_.valid_ == m_buffer_.offset_);
    YETI_ASSERT(m_buffer_.size_ != 0);

    if (m_buffer_.data_ == NULL) {
        m_buffer_.data_ = new YETI_Byte[m_buffer_.size_];
        if (m_buffer_.data_ == NULL) return YETI_ERROR_OUT_OF_MEMORY;
    }

    m_buffer_.offset_ = 0;
    YETI_Result result = m_source_->read(m_buffer_.data_, m_buffer_.size_, &m_buffer_.valid_);
    if (YETI_FAILED(result)) m_buffer_.valid_ = 0;
    return result;
}

YETI_Result BufferedInputStream::release_buffer()
{
    YETI_ASSERT(m_buffer_.size == 0);
    YETI_ASSERT(m_buffer_.offset_ == m_buffer_.valid_);

    delete []m_buffer_.data_;
    m_buffer_.data_ = NULL;
    m_buffer_.offset_ = 0;
    m_buffer_.valid_ = 0;

    return YETI_SUCCESS;
}

YETI_Result BufferedInputStream::peek(void * buffer, YETI_Size  bytes_to_read, YETI_Size * bytes_read);

YETI_Result BufferedInputStream::read(void * buffer, YETI_Size  bytes_to_read, YETI_Size * bytes_read /*= NULL*/);
YETI_Result BufferedInputStream::seek(YETI_Position offset);
YETI_Result BufferedInputStream::tell(YETI_Position & offset);
YETI_Result BufferedInputStream::get_size(YETI_LargeSize & size);
YETI_Result BufferedInputStream::get_available(YETI_LargeSize & available);

NAMEEND
