#ifndef _CXL_YETI_STREAMS_H_
#define _CXL_YETI_STREAMS_H_

#include "YetiTypes.h"
#include "YetiReferences.h"
#include "YetiConstants.h"
#include "YetiResults.h"
#include "YetiDataBuffer.h"
#include "YetiString.h"

NAMEBEG

class String;

const int YETI_ERROR_READ_FAILED    = YETI_ERROR_BASE_IO - 0;
const int YETI_ERROR_WRITE_FAILED   = YETI_ERROR_BASE_IO - 1;
const int YETI_ERROR_READ_EOS       = YETI_ERROR_BASE_IO - 2;

class InputStream
{
public:
    virtual ~InputStream() {}

    virtual YETI_Result load(DataBuffer & buffer, YETI_Size max_read = 0);
    virtual YETI_Result read(void * buffer,
        YETI_Size bytes_to_read,
        YETI_Size bytes_read = NULL) = 0;
    virtual YETI_Result read_fully(void * buffer,
        YETI_Size bytes_to_read) = 0;
    virtual YETI_Result seek(YETI_Position offset) = 0;
    virtual YETI_Result skip(YETI_Size offset);
    virtual YETI_Result tell(YETI_Position & offset) = 0;
    virtual YETI_Result get_size(YETI_LargeSize & size) = 0;
    virtual YETI_Result get_available(YETI_LargeSize & available) = 0;

    YETI_Result read_ui64(YETI_UInt64 & value);
    YETI_Result read_ui32(YETI_UInt32 & value);
    YETI_Result read_ui24(YETI_UInt32 & value);
    YETI_Result read_ui16(YETI_UInt16 & value);
    YETI_Result read_ui08(YETI_UInt8 & value);
};

typedef Reference<InputStream> InputStreamReference;

class OutputStream
{
public:
    virtual ~OutputStream() {}

    virtual YETI_Result write(const void * buffer,
        YETI_Size bytes_to_write,
        YETI_Size * bytes_written = NULL) = 0;
    virtual YETI_Result write_fully(const void * buffer,
        YETI_Size bytes_to_write);
    virtual YETI_Result write_string(const char * string_buffer);
    virtual YETI_Result write_line(const char * line_buffer);
    virtual YETI_Result seek(YETI_Position offset) = 0;
    virtual YETI_Result tell(YETI_Position & offset) = 0;
    virtual YETI_Result flush() { return YETI_SUCCESS; }

    YETI_Result write_ui64(YETI_UInt64 value);
    YETI_Result write_ui32(YETI_UInt32 value);
    YETI_Result write_ui24(YETI_UInt32 value);
    YETI_Result write_ui16(YETI_UInt16 value);
    YETI_Result write_ui08(YETI_UInt8 value);
};

typedef Reference<OutputStream> OutputStreamReference;

YETI_Result stream_to_stream_copy(InputStream & from,
                                  OutputStream & to,
                                  YETI_Position offset = 0,
                                  YETI_LargeSize size = 0,
                                  YETI_LargeSize * bytes_written = NULL);

class DelegatingInputStream : public InputStream
{
public:
    YETI_Result seek(YETI_Position offset)
    {
        return _input_seek(offset);
    }
    YETI_Result tell(YETI_Position & offset)
    {
        return _input_tell(offset);
    }

private:
    virtual YETI_Result _input_seek(YETI_Position offset) = 0;
    virtual YETI_Result _input_tell(YETI_Position & offset) = 0;
};

class DelegatingOutputStream : public OutputStream
{
public:
    YETI_Result seek(YETI_Position offset)
    {
        return _output_seek(offset);
    }
    YETI_Result tell(YETI_Position & offset)
    {
        return _output_tell(offset);
    }

private:
    virtual YETI_Result _output_seek(YETI_Position offset) = 0;
    virtual YETI_Result _output_tell(YETI_Position & offset) = 0;
};

class MemoryStream : public DelegatingInputStream, public DelegatingOutputStream
{
public:
    MemoryStream(YETI_Size initial_capacity = 0);
    MemoryStream(const void * data, YETI_Size size);
    virtual ~MemoryStream() {}

    const DataBuffer & get_buffer() const { return m_buffer_; }

    YETI_Result read(void * buffer,
        YETI_Size bytes_to_read,
        YETI_Size * bytes_read = NULL);
    YETI_Result get_size(YETI_LargeSize & size)
    {
        size = m_buffer_.get_data_size();
        return YETI_SUCCESS;
    }
    YETI_Result get_available(YETI_LargeSize & available)
    {
        available = (YETI_LargeSize)m_buffer_.get_data_size() - m_read_offset_;
        return YETI_SUCCESS;
    }

    YETI_Result write(const void * buffer,
        YETI_Size bytes_to_write,
        YETI_Size * bytes_written = NULL);
    const YETI_Byte * get_data() const { return m_buffer_.get_data(); }
    YETI_Byte * use_data() { return m_buffer_.use_data(); }
    YETI_Size get_data_size() const { return m_buffer_.get_data_size(); }
    YETI_Size get_buffer_size() const { return m_buffer_.get_buffer_size(); }

    YETI_Result set_data_size(YETI_Size size);

private:
    YETI_Result _input_seek(YETI_Position offset);
    YETI_Result _input_tell(YETI_Position & offset)
    {
        offset = m_read_offset_;
        return YETI_SUCCESS;
    }

    YETI_Result _output_seek(YETI_Position offset);
    YETI_Result _output_tell(YETI_Position & offset)
    {
        offset = m_write_offset_;
        return YETI_SUCCESS;
    }

protected:
    DataBuffer m_buffer_;
    YETI_Size m_read_offset_;
    YETI_Size m_write_offset_;
};

typedef Reference<MemoryStream> MemoryStreamReference;

class StringOutputStream : public OutputStream
{
public:
    StringOutputStream(YETI_Size size = 4096);
    StringOutputStream(String * storage);
    virtual ~StringOutputStream();

    const String & get_string() const { return *m_string_; }
    YETI_Result reset() { if (m_string_) m_string_->set_length(0); return YETI_SUCCESS; }
    YETI_Result write(const void * buffer, YETI_Size bytes_to_write, YETI_Size * bytes_written = NULL);
    YETI_Result seek(YETI_Position /*offset*/) { return YETI_ERROR_NOT_SUPPORTED; }
    YETI_Result tell(YETI_Position & offset) { offset = m_string_->get_length(); return YETI_SUCCESS; }

protected:
    String * m_string_;
    bool m_string_is_owned;
};

typedef Reference<StringOutputStream> StringOutputStreamReference;

class SubInputStream : public InputStream
{
public:
    SubInputStream(InputStreamReference & source,
        YETI_Position  start,
        YETI_LargeSize size);

    virtual YETI_Result read(void * buffer, YETI_Size bytes_to_read, YETI_Size bytes_read = NULL) = 0;
    virtual YETI_Result seek(YETI_Position offset) = 0;
    virtual YETI_Result tell(YETI_Position & offset) = 0;
    virtual YETI_Result get_available(YETI_LargeSize & available) = 0;

private:
    InputStreamReference m_source_;
    YETI_Position m_position_;
    YETI_Position m_start_;
    YETI_LargeSize m_size_;
};

class NullOutputStream : public OutputStream
{
public:
    NullOutputStream() {};
    virtual ~NullOutputStream() {}

    YETI_Result write(const void * buffer, YETI_Size bytes_to_write, YETI_Size * bytes_written = NULL);
    YETI_Result seek(YETI_Position /*offset*/) { return YETI_ERROR_NOT_SUPPORTED; }
    YETI_Result tell(YETI_Position & /*offset*/) { return YETI_ERROR_NOT_SUPPORTED; }
};

typedef Reference<NullOutputStream> NullOutputStreamReference;

NAMEEND

#endif // _CXL_YETI_STREAMS_H_
