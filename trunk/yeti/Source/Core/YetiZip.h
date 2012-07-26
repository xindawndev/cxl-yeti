#ifndef _CXL_YETI_ZIP_H_
#define _CXL_YETI_ZIP_H_

#include "YetiConfig.h"
#include "YetiStreams.h"
#include "YetiFile.h"

NAMEBEG

class ZipInflateState;
class ZipDeflateState;

const int YETI_ZIP_COMPRESSION_LEVEL_DEFAULT = -1;
const int YETI_ZIP_COMPRESSION_LEVEL_MIN     = 0;
const int YETI_ZIP_COMPRESSION_LEVEL_MAX     = 9;
const int YETI_ZIP_COMPRESSION_LEVEL_NONE    = 0;

class Zip
{
public:
    static YETI_Result map_error(int err);

    typedef enum {
        ZLIB,
        GZIP
    } TypeFormat;

    static YETI_Result compress_deflate(const DataBuffer & in,
        DataBuffer & out,
        int compression_level = YETI_ZIP_COMPRESSION_LEVEL_DEFAULT,
        TypeFormat format = ZLIB);
    static YETI_Result decompress_inflate(const DataBuffer & in,
        DataBuffer & out);
    static YETI_Result compress_deflate(File &in,
        File & out,
        int compression_level = YETI_ZIP_COMPRESSION_LEVEL_DEFAULT,
        TypeFormat format = GZIP);
};

class ZipInflatingInputStream : public InputStream
{
public:
    ZipInflatingInputStream(InputStreamReference & source);
    ~ZipInflatingInputStream();

    virtual YETI_Result read(void * buffer,
        YETI_Size bytes_to_read,
        YETI_Size * bytes_read = NULL);
    virtual YETI_Result seek(YETI_Position offset);
    virtual YETI_Result tell(YETI_Position & offset);
    virtual YETI_Result get_size(YETI_LargeSize & size);
    virtual YETI_Result get_available(YETI_LargeSize & available);

private:
    InputStreamReference m_source_;
    YETI_Position m_position_;
    ZipInflateState * m_state_;
    DataBuffer m_buffer_;
};

class ZipDeflatingInputStream : public InputStream
{
public:
    ZipDeflatingInputStream(InputStreamReference & source,
        int compression_level = YETI_ZIP_COMPRESSION_LEVEL_DEFAULT,
        Zip::TypeFormat format = Zip::ZLIB);
    ~ZipDeflatingInputStream();

    virtual YETI_Result read(void * buffer,
        YETI_Size bytes_to_read,
        YETI_Size * bytes_read = NULL);
    virtual YETI_Result seek(YETI_Position offset);
    virtual YETI_Result tell(YETI_Position & offset);
    virtual YETI_Result get_size(YETI_LargeSize & size);
    virtual YETI_Result get_available(YETI_LargeSize & available);

private:
    InputStreamReference m_source_;
    YETI_Position m_position_;
    bool m_eos_;
    ZipDeflateState * m_state_;
    DataBuffer m_buffer_;
};

NAMEEND

#endif // _CXL_YETI_ZIP_H_
