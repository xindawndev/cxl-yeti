#ifndef _CXL_YETI_DATA_BUFFER_H_
#define _CXL_YETI_DATA_BUFFER_H_

#include "YetiTypes.h"
#include "YetiConstants.h"

NAMEBEG

class DataBuffer 
{
public:
    DataBuffer();
    DataBuffer(YETI_Size size);
    DataBuffer(const void * data, YETI_Size size, bool copy = false);
    DataBuffer(const DataBuffer & other);
    ~DataBuffer();

    DataBuffer & operator =(const DataBuffer & copy);
    bool         operator==(const DataBuffer & other) const;

    virtual YETI_Result set_buffer(YETI_Byte * buffer, YETI_Size buffer_size);
    virtual YETI_Result set_buffer_size(YETI_Size size);
    virtual YETI_Size   get_buffer_size() const { return m_buffer_size_; }
    virtual YETI_Result reserve(YETI_Size size);
    virtual YETI_Result clear();

    virtual const YETI_Byte * get_data() const { return m_buffer_; }
    virtual YETI_Byte *       use_data() { return m_buffer_; }
    virtual YETI_Size         get_data_size() const { return m_data_size_; }
    virtual YETI_Result       set_data_size(YETI_Size size);
    virtual YETI_Result       set_data(const YETI_Byte * data, YETI_Size data_size);

protected:
    bool        m_buffer_is_local_;
    YETI_Byte * m_buffer_;
    YETI_Size   m_buffer_size_;
    YETI_Size   m_data_size_;

    YETI_Result _reallocate_buffer(YETI_Size size);
};

NAMEEND

#endif // _CXL_YETI_DATA_BUFFER_H_
