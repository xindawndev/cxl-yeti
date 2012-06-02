#include "YetiDataBuffer.h"
#include "YetiUtil.h"
#include "YetiResults.h"

NAMEBEG

DataBuffer::DataBuffer()
: m_buffer_is_local_(true)
, m_buffer_(NULL)
, m_buffer_size_(0)
, m_data_size_(0)
{
}

DataBuffer::DataBuffer(YETI_Size size)
: m_buffer_is_local_(true)
, m_buffer_(size ? new YETI_Byte[size] : NULL)
, m_buffer_size_(size)
, m_data_size_(0)
{
}

DataBuffer::DataBuffer(const void * data, YETI_Size size, bool copy)
: m_buffer_is_local_(copy)
, m_buffer_(copy ? (size ? new YETI_Byte[size] : NULL) : reinterpret_cast<YETI_Byte *>(const_cast<void *>(data)))
, m_buffer_size_(size)
, m_data_size_(size)
{
    if (copy && size) MemoryCopy(m_buffer_, data, size);
}

DataBuffer::DataBuffer(const DataBuffer & other)
: m_buffer_is_local_(true)
, m_buffer_(NULL)
, m_buffer_size_(other.m_data_size_)
{
    if (m_buffer_size_) {
        m_buffer_ = new YETI_Byte[m_buffer_size_];
        MemoryCopy(m_buffer_, other.m_buffer_, m_buffer_size_);
    }
}

DataBuffer::~DataBuffer()
{
    clear();
}

YETI_Result DataBuffer::clear()
{
    if (m_buffer_is_local_) {
        delete[] m_buffer_;
    }

    m_buffer_ = NULL;
    m_data_size_ = 0;
    m_buffer_size_ = 0;

    return YETI_SUCCESS;
}

DataBuffer & DataBuffer::operator =(const DataBuffer & copy)
{
    if (this != &copy) {
        clear();

        m_buffer_is_local_ = true;
        m_buffer_size_ = copy.m_buffer_size_;
        m_data_size_ = copy.m_data_size_;

        if (m_buffer_size_) {
            m_buffer_ = new YETI_Byte[m_buffer_size_];
            MemoryCopy(m_buffer_, copy.m_buffer_, m_buffer_size_);
        }
    }

    return *this;
}

bool DataBuffer::operator ==(const DataBuffer & other) const
{
    if (m_data_size_ != other.m_data_size_) return false;

    return MemoryEqual(m_buffer_, other.m_buffer_, m_data_size_);
}

YETI_Result DataBuffer::set_buffer(YETI_Byte * buffer, YETI_Size buffer_size)
{
    clear();
    m_buffer_is_local_ = false;
    m_buffer_ = buffer;
    m_buffer_size_ = buffer_size;
    m_data_size_ = 0;

    return YETI_SUCCESS;
}

YETI_Result DataBuffer::set_buffer_size(YETI_Size size)
{
    if (m_buffer_is_local_) {
        return _reallocate_buffer(size);
    } else {
        return YETI_ERROR_NOT_SUPPORTED;
    }
}

YETI_Result DataBuffer::reserve(YETI_Size size)
{
    if (size <= m_buffer_size_) return YETI_SUCCESS;

    YETI_Size new_size = m_buffer_size_ * 2;
    if (new_size < size) new_size = size;

    return set_buffer_size(new_size);
}

YETI_Result DataBuffer::set_data_size(YETI_Size size)
{
    if (size > m_buffer_size_) {
        if (m_buffer_is_local_) {
            YETI_CHECK(_reallocate_buffer(size));
        } else {
            return YETI_ERROR_NOT_SUPPORTED;
        }
    }

    m_data_size_ = size;

    return YETI_SUCCESS;
}

YETI_Result DataBuffer::set_data(const YETI_Byte * data, YETI_Size data_size)
{
    if (data_size > m_buffer_size_) {
        if (m_buffer_is_local_) {
            YETI_CHECK(_reallocate_buffer(data_size));
        } else {
            return YETI_ERROR_NOT_SUPPORTED;
        }
    }

    if (data) MemoryCopy(m_buffer_, data, data_size);
    m_data_size_ = data_size;

    return YETI_SUCCESS;
}

YETI_Result DataBuffer::_reallocate_buffer(YETI_Size size)
{
    if (m_data_size_ > size) return YETI_ERROR_INVALID_PARAMETERS;

    YETI_Byte * new_buffer = new YETI_Byte[size];

    if (m_buffer_ && m_data_size_) {
        MemoryCopy(new_buffer, m_buffer_, m_data_size_);
    }

    delete[] m_buffer_;
    m_buffer_ = new_buffer;
    m_buffer_size_ = size;

    return YETI_SUCCESS;
}

NAMEEND
