#include "YetiStreams.h"

#include "YetiUtil.h"
#include "YetiConstants.h"
#include "YetiString.h"
#include "YetiDebug.h"

NAMEBEG

const YETI_Size YETI_INPUT_STREAM_LOAD_DEFAULT_READ_CHUNK = 4096;
const YETI_LargeSize YETI_INPUT_STREAM_LOAD_MAX_SIZE      = 0x40000000;

YETI_Result InputStream::load(DataBuffer & buffer, YETI_Size max_read /* = 0 */)
{
    YETI_Result result;
    YETI_LargeSize total_bytes_read;

    buffer.set_data_size(0);

    if (max_read > YETI_INPUT_STREAM_LOAD_MAX_SIZE) {
        return YETI_ERROR_INVALID_PARAMETERS;
    }

    YETI_LargeSize size;
    if (YETI_SUCCEEDED(get_size(size))) {
        if (max_read && max_read < size) size = max_read;
        if (size > YETI_INPUT_STREAM_LOAD_MAX_SIZE) {
            return YETI_ERROR_OUT_OF_RANGE;
        }
    } else {
        size = max_read;
    }

    if (size) YETI_CHECK(buffer.reserve((YETI_Size)size));

    total_bytes_read = 0;
    do {
        YETI_LargeSize available = 0;
        YETI_LargeSize bytes_to_read;
        YETI_Size bytes_read;
        YETI_Byte * data;

        result = get_available(available);
        if (YETI_SUCCEEDED(result) && available) {
            bytes_to_read = available;
        } else {
            bytes_to_read =  size - total_bytes_read;
        }

        if (size != 0 && total_bytes_read + bytes_to_read > size) {
            bytes_to_read = YETI_INPUT_STREAM_LOAD_DEFAULT_READ_CHUNK;
        }

        if (bytes_to_read == 0) break;

        if (total_bytes_read + bytes_to_read > YETI_INPUT_STREAM_LOAD_MAX_SIZE) {
            buffer.set_buffer_size(0);
            return YETI_ERROR_OUT_OF_RANGE;
        }
        YETI_CHECK(buffer.reserve((YETI_Size)(total_bytes_read + bytes_to_read)));

        data = buffer.use_data() + total_bytes_read;
        result = read((void *)data, (YETI_Size)bytes_to_read, &bytes_read);
        if (YETI_SUCCEEDED(result) && bytes_read != 0) {
            total_bytes_read +=bytes_read;
            buffer.set_data_size((YETI_Size)total_bytes_read);
        }
    } while(YETI_SUCCEEDED(result) && (size == 0 || total_bytes_read < size));

    if (result == YETI_ERROR_EOS) {
        return YETI_SUCCESS;
    }

    return result;
}

YETI_Result InputStream::read_fully(void * buffer, YETI_Size bytes_to_read)
{
    if (bytes_to_read == 0) return YETI_SUCCESS;

    YETI_Size bytes_read;
    while (bytes_to_read) {
        YETI_Result result = read(buffer, bytes_to_read, &bytes_read);
        if (YETI_FAILED(result)) return result;
        if (bytes_read == 0) return YETI_ERROR_INTERNAL;
        YETI_ASSERT(bytes_read <= bytes_to_read);
        bytes_to_read -= bytes_read;
        buffer = (void *)(((YETI_Byte *)buffer) + bytes_read);
    }
    return YETI_SUCCESS;
}

YETI_Result InputStream::read_ui64(YETI_UInt64 & value)
{
    unsigned char buffer[8];

    YETI_Result result;
    result = read_fully((void *)buffer, 8);
    if (YETI_FAILED(result)) {
        value = 0;
        return result;
    }

    value = bytes_to_int64_be(buffer);
    return YETI_SUCCESS;
}

YETI_Result InputStream::read_ui32(YETI_UInt32 & value)
{
    unsigned char buffer[4];

    YETI_Result result;
    result = read_fully((void *)buffer, 4);
    if (YETI_FAILED(result)) {
        value = 0;
        return result;
    }

    value = bytes_to_int32_be(buffer);
    return YETI_SUCCESS;
}

YETI_Result InputStream::read_ui24(YETI_UInt32 & value)
{
    unsigned char buffer[3];

    YETI_Result result;
    result = read_fully((void *)buffer, 3);
    if (YETI_FAILED(result)) {
        value = 0;
        return result;
    }

    value = bytes_to_int24_be(buffer);
    return YETI_SUCCESS;
}

YETI_Result InputStream::read_ui16(YETI_UInt16 & value)
{
    unsigned char buffer[2];

    YETI_Result result;
    result = read_fully((void *)buffer, 2);
    if (YETI_FAILED(result)) {
        value = 0;
        return result;
    }

    value = bytes_to_int16_be(buffer);
    return YETI_SUCCESS;
}

YETI_Result InputStream::read_ui08(YETI_UInt8 & value)
{
    unsigned char buffer[1];

    YETI_Result result;
    result = read_fully((void *)buffer, 1);
    if (YETI_FAILED(result)) {
        value = 0;
        return result;
    }

    value = buffer[0];
    return YETI_SUCCESS;
}

YETI_Result InputStream::skip(YETI_Size count)
{
    YETI_Position position;
    YETI_CHECK(tell(position));
    return seek(position + count);
}

YETI_Result OutputStream::write_fully(const void * buffer, YETI_Size bytes_to_write)
{
    if (bytes_to_write == 0) return YETI_SUCCESS;

    YETI_Size bytes_written;
    while (bytes_to_write) {
        YETI_Result result = write(buffer, bytes_to_write, &bytes_written);
        if (YETI_FAILED(result)) return result;
        if (bytes_written == 0) return YETI_ERROR_INTERNAL;
        YETI_ASSERT(bytes_written <= bytes_to_write);
        bytes_to_write -= bytes_written;
        buffer = (const void *)(((const YETI_Byte *)buffer) + bytes_written);
    }

    return YETI_SUCCESS;
}

YETI_Result OutputStream::write_string(const char * buffer)
{
    YETI_Size string_length;
    if (buffer == NULL || (string_length = StringLength(buffer)) == 0)  {
        return YETI_SUCCESS;
    }

    return write_fully((const void *)buffer, string_length);
}

YETI_Result OutputStream::write_line(const char * buffer)
{
    YETI_CHECK(write_string(buffer));
    YETI_CHECK(write_fully((const void *)"\r\n", 2));
    return YETI_SUCCESS;
}

YETI_Result OutputStream::write_ui64(YETI_UInt64 value)
{
    unsigned char buffer[8];
    bytes_from_int64_be(buffer, value);
    return write_fully((void *)buffer, 8);
}

YETI_Result OutputStream::write_ui32(YETI_UInt32 value)
{
    unsigned char buffer[4];
    bytes_from_int64_be(buffer, value);
    return write_fully((void *)buffer, 4);
}

YETI_Result OutputStream::write_ui24(YETI_UInt32 value)
{
    unsigned char buffer[3];
    bytes_from_int64_be(buffer, value);
    return write_fully((void *)buffer, 3);
}

YETI_Result OutputStream::write_ui16(YETI_UInt16 value)
{
    unsigned char buffer[2];
    bytes_from_int64_be(buffer, value);
    return write_fully((void *)buffer, 2);
}

YETI_Result OutputStream::write_ui08(YETI_UInt8 value)
{
    return write_fully((void *)value, 1);
}

MemoryStream::MemoryStream(YETI_Size initial_capacity)
: m_buffer_(initial_capacity)
, m_read_offset_(0)
, m_write_offset_(0)
{
}

YETI_Result MemoryStream::read(void * buffer,
                               YETI_Size bytes_to_read,
                               YETI_Size * bytes_read/* = NULL*/)
{
    if (bytes_to_read == 0) {
        if (bytes_read) *bytes_read = 0;
        return YETI_SUCCESS;
    }

    YETI_Size available = m_buffer_.get_data_size();
    if (m_read_offset_ + bytes_to_read > available) {
        bytes_to_read = available - m_read_offset_;
    }

    if (bytes_to_read) {
        MemoryCopy(buffer, (void *)(((char *)m_buffer_.use_data()) + m_read_offset_), bytes_to_read);
        m_read_offset_ += bytes_to_read;
    }
    if (bytes_to_read) {
        *bytes_read = bytes_to_read;
    }

    return bytes_to_read ? YETI_SUCCESS : YETI_ERROR_EOS;
}

YETI_Result MemoryStream::_input_seek(YETI_Position offset)
{
    if (offset > m_buffer_.get_data_size()) {
        return YETI_ERROR_OUT_OF_RANGE;
    } else {
        m_read_offset_ = (YETI_Size)offset;
        return YETI_SUCCESS;
    }
}

YETI_Result MemoryStream::write(const void * buffer, YETI_Size bytes_to_write, YETI_Size * bytes_written /* = NULL */)
{
    YETI_CHECK(m_buffer_.reserve(m_write_offset_ + bytes_to_write));

    MemoryCopy(m_buffer_.use_data() + m_write_offset_, buffer, bytes_to_write);
    m_write_offset_ += bytes_to_write;
    if (m_write_offset_ > m_buffer_.get_data_size()) {
        m_buffer_.set_data_size(m_write_offset_);
    }
    if (bytes_written) *bytes_written = bytes_to_write;

    return YETI_SUCCESS;
}

YETI_Result MemoryStream::_output_seek(YETI_Position offset)
{
    if (offset <= m_buffer_.get_data_size()) {
        m_write_offset_ = (YETI_Size)offset;
        return YETI_SUCCESS;
    }

    return YETI_ERROR_OUT_OF_RANGE;
}

YETI_Result MemoryStream::set_data_size(YETI_Size size)
{
    YETI_CHECK(m_buffer_.set_data_size(size));

    if (m_read_offset_ > size) m_read_offset_ = size;
    if (m_write_offset_ > size) m_write_offset_ = size;

    return YETI_SUCCESS;
}

const unsigned int YETI_STREAM_COPY_BUFFER_SIZE = 65536;

YETI_Result stream_to_stream_copy(InputStream & from,
                                  OutputStream & to,
                                  YETI_Position offset /*= 0*/,
                                  YETI_LargeSize size /*= 0*/,
                                  YETI_LargeSize * bytes_written/* = NULL*/)
{
    if (bytes_written) *bytes_written = 0;
    if (offset) YETI_CHECK(from.seek(offset));

    YETI_LargeSize bytes_transfered = 0;
    YETI_Byte * buffer = new YETI_Byte[YETI_STREAM_COPY_BUFFER_SIZE];
    YETI_Result result = YETI_SUCCESS;
    if (buffer == NULL) return YETI_ERROR_OUT_OF_MEMORY;

    for (;;) {

    }

end:
    delete[] buffer;
    return result;
}

NAMEEND
