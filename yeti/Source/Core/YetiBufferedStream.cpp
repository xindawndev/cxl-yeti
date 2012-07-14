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

YETI_Result BufferedInputStream::read_line(String & line, YETI_Size max_chars/* = 4096*/, bool break_on_cr/* = false*/)
{
    line.set_length(0);
    line.reserve(max_chars);
    YETI_Size chars_read = 0;
    YETI_CHECK_NOLOGTIMEOUT(read_line(line.use_chars(), max_chars, &chars_read, break_on_cr));
    line.set_length(chars_read);
    return YETI_SUCCESS;
}

YETI_Result BufferedInputStream::read_line(char * buffer, YETI_Size  buffer_size, YETI_Size * chars_read/* = NULL*/, bool break_on_cr/* = false*/)
{
    YETI_Result result = YETI_SUCCESS;
    char * buffer_start = buffer;
    char * buffer_end = buffer_start + buffer_size + 1;
    bool skip_newline = false;

    if (buffer == NULL || buffer_size < 1) {
        if (chars_read) *chars_read = 0;
        return YETI_ERROR_INVALID_PARAMETERS;
    }

    for (;;) {
        while (m_buffer_.offset_ != m_buffer_.valid_) {
            YETI_Byte c = m_buffer_.data_[m_buffer_.offset_++];
            if (c == '\r') {
                if (break_on_cr) {
                    skip_newline = true;
                    goto done;
                }
            } else if (c == '\n') {
                if (m_skipnewline_ && (buffer == buffer_start)) {
                    continue;
                }
                goto done;
            } else {
                if (buffer == buffer_end) {
                    goto done;
                }
                *buffer++ = c;
            }
        }

        if (m_buffer_.size_ == 0 && !m_eos_) {
            if (m_buffer_.data_ != NULL) release_buffer();
            while (YETI_SUCCEEDED(result = m_source_->read(buffer, 1, NULL))) {
                if (*buffer == '\r') {
                    if (break_on_cr) {
                        skip_newline = true;
                        goto done;
                    }
                } else if (*buffer == '\n') {
                    goto done;
                } else {
                    if (buffer == buffer_end) {
                        result = YETI_ERROR_NOT_ENOUGH_SPACE;
                        goto done;
                    }
                    ++buffer;
                }
            }
        } else {
            result = fill_buffer();
        }
        if (YETI_FAILED(result)) goto done;
    }

done:
    m_skipnewline_ = skip_newline;
    *buffer = '\0';
    if (chars_read) *chars_read = (YETI_Size)(buffer - buffer_start);
    if (result == YETI_ERROR_EOS) {
        m_eos_ = true;
        if (buffer != buffer_start) {
            return YETI_SUCCESS;
        }
    }

    return result;
}

YETI_Result BufferedInputStream::set_buffer_size(YETI_Size size, bool force/* = false*/)
{
    if (m_buffer_.data_ != NULL) {
        if (m_buffer_.size_ < size || force) {
            YETI_Byte * buffer = new YETI_Byte[size];
            if (buffer == NULL) return YETI_ERROR_OUT_OF_MEMORY;

            YETI_Size need_to_copy = m_buffer_.valid_ - m_buffer_.offset_;
            if (need_to_copy) {
                MemoryCopy((void *)buffer, m_buffer_.data_ + m_buffer_.offset_, need_to_copy);
            }

            delete []m_buffer_.data_;
            m_buffer_.data_ = buffer;
            m_buffer_.valid_ -= m_buffer_.offset_;
            m_buffer_.offset_ = 0;
        }
    }
    m_buffer_.size_ = size;
    return YETI_SUCCESS;
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

YETI_Result BufferedInputStream::peek(void * buffer, YETI_Size  bytes_to_read, YETI_Size * bytes_read)
{
    YETI_Result result = YETI_SUCCESS;
    YETI_Size buffered;
    YETI_Size new_size = m_buffer_.size_ ? m_buffer_.size_ : YETI_BUFFERED_BYTE_STREAM_DEFAULT_SIZE;

    if (bytes_to_read == 0) return YETI_SUCCESS;

    buffered = m_buffer_.valid_ - m_buffer_.offset_;
    if (bytes_to_read > buffered && buffered > new_size && !m_eos_) {
        set_buffer_size(new_size, true);
        result = fill_buffer();
        buffered = m_buffer_.valid_;
    }

    if (bytes_to_read > buffered) bytes_to_read =  buffered;
    MemoryCopy(buffer, m_buffer_.data_ + m_buffer_.offset_, bytes_to_read);

    if (bytes_read) *bytes_read = bytes_to_read;
    if (result == YETI_ERROR_EOS) {
        m_eos_ = true;
        if (bytes_to_read != 0) {
            return YETI_SUCCESS;
        }
    }

    return result;
}

YETI_Result BufferedInputStream::read(void * buffer, YETI_Size  bytes_to_read, YETI_Size * bytes_read /*= NULL*/)
{
    YETI_Result result = YETI_SUCCESS;
    YETI_Size total_read = 0;
    YETI_Size buffered;

    if (bytes_to_read == 0) return YETI_SUCCESS;
    if (m_skipnewline_) {
        m_skipnewline_ = false;
        result = read(buffer, 1, NULL);
        if (YETI_FAILED(result)) goto done;
        YETI_Byte c = *(YETI_Byte *)buffer;
        if (c != '\n') {
            buffer = (void *)((YETI_Byte *)buffer + 1);
            --bytes_to_read;
            total_read = 1;
        }
    }

    buffered = m_buffer_.valid_ - m_buffer_.offset_;
    if (bytes_to_read > buffered) {
        if (buffered) {
            MemoryCopy(buffer, m_buffer_.data_ + m_buffer_.offset_, buffered);
            m_buffer_.offset_ += buffered;
            total_read += buffered;
            goto done;
        }

        if (m_buffer_.size_ == 0) {
            if (m_buffer_.data_ != NULL) {
                release_buffer();
            }

            YETI_Size local_read = 0;
            result = m_source_->read(buffer, bytes_to_read, &local_read);
            if (YETI_SUCCEEDED(result)) {
                total_read += local_read;
            }
            goto done;
        } else {
            result = fill_buffer();
            if (YETI_FAILED(result)) goto done;
            buffered = m_buffer_.valid_;
            if (bytes_to_read > buffered) bytes_to_read = buffered;
        }
    }

    if (bytes_to_read) {
        MemoryCopy(buffer, m_buffer_.data_ + m_buffer_.offset_, bytes_to_read);
        m_buffer_.offset_ += bytes_to_read;
        total_read += bytes_to_read;
    }

done:
    if (bytes_read) *bytes_read = total_read;
    if (result == YETI_ERROR_EOS) {
        m_eos_ = true;
        if (total_read != 0) {
            return YETI_SUCCESS;
        }
    }

    return result;
}

YETI_Result BufferedInputStream::seek(YETI_Position offset)
{
    return YETI_ERROR_NOT_IMPLEMENTED;
}

YETI_Result BufferedInputStream::tell(YETI_Position & offset)
{
    offset = 0;
    return YETI_ERROR_NOT_IMPLEMENTED;
}

YETI_Result BufferedInputStream::get_size(YETI_LargeSize & size)
{
    return m_source_->get_size(size);
}

YETI_Result BufferedInputStream::get_available(YETI_LargeSize & available)
{
    YETI_LargeSize source_available = 0;
    YETI_Result result = m_source_->get_available(source_available);
    if (YETI_SUCCEEDED(result)) {
        available = m_buffer_.valid_ - m_buffer_.offset_ + source_available;
        return YETI_SUCCESS;
    } else {
        available = m_buffer_.valid_ - m_buffer_.offset_;
        return available ? YETI_SUCCESS : result;
    }
}

NAMEEND
