#include "YetiConfig.h"
#include "YetiZip.h"
#include "YetiLogging.h"
#include "YetiUtil.h"

#include "zlib.h"

YETI_SET_LOCAL_LOGGER("yeti.zip")

NAMEBEG

const unsigned int YETI_ZIP_DEFAULT_BUFFER_SIZE = 4096;

YETI_Result Zip::map_error(int err)
{
    switch (err)
    {
    case Z_OK:            return YETI_SUCCESS;
    case Z_STREAM_END:    return YETI_ERROR_EOS;
    case Z_DATA_ERROR:
    case Z_STREAM_ERROR:  return YETI_ERROR_INVALID_FORMAT;
    case Z_MEM_ERROR:     return YETI_ERROR_OUT_OF_MEMORY;
    case Z_VERSION_ERROR: return YETI_ERROR_INTERNAL;
    case Z_NEED_DICT:     return YETI_ERROR_NOT_SUPPORTED;
    default:              return YETI_FAILURE;
    }
}

class ZipInflateState
{
public:
    ZipInflateState() {
        SetMemory(&m_stream_, 0, sizeof(m_stream_));
        inflateInit2(&m_stream_, 15 + 32); // // 15 = default window bits, +32 = automatic header
    }

    ~ZipInflateState() {
        inflateEnd(&m_stream_);
    }

    z_stream m_stream_;
};

class ZipDeflateState 
{
public:
    ZipDeflateState(int compression_level,
        Zip::TypeFormat format) {
            if (compression_level < YETI_ZIP_COMPRESSION_LEVEL_DEFAULT ||
                compression_level > YETI_ZIP_COMPRESSION_LEVEL_MAX) {
                    compression_level = YETI_ZIP_COMPRESSION_LEVEL_DEFAULT;
            }

            SetMemory(&m_stream_, 0, sizeof(m_stream_));
            deflateInit2(&m_stream_,
                compression_level,
                Z_DEFLATED,
                15 + (format == Zip::GZIP ? 16 : 0),
                8,
                Z_DEFAULT_STRATEGY);
    }

    ~ZipDeflateState() {
        deflateEnd(&m_stream_);
    }

    z_stream m_stream_;
};

ZipInflatingInputStream::ZipInflatingInputStream(InputStreamReference & source)
: m_source_(source)
, m_position_(0)
, m_state_(new ZipInflateState())
, m_buffer_(YETI_ZIP_DEFAULT_BUFFER_SIZE) {

}

ZipInflatingInputStream::~ZipInflatingInputStream()
{
    delete m_state_;
    m_state_ = NULL;
}

YETI_Result ZipInflatingInputStream::read(void * buffer,
                                          YETI_Size bytes_to_read,
                                          YETI_Size * bytes_read /*= NULL*/)
{
    if (m_state_ == NULL) return YETI_ERROR_INVALID_STATE;
    if (buffer == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    if (bytes_to_read == 0) return YETI_SUCCESS;

    if (bytes_read) *bytes_read = 0;

    m_state_->m_stream_.next_out = (Bytef *)buffer;
    m_state_->m_stream_.avail_out = (uInt)bytes_to_read;

    while (m_state_->m_stream_.avail_out) {
        int err = inflate(&m_state_->m_stream_, Z_NO_FLUSH);
        if (err == Z_STREAM_END) {
            break;
        } else if (err == Z_OK) {
            continue;
        } else if (err == Z_BUF_ERROR) {
            YETI_Size input_bytes_read = 0;
            YETI_Result result = m_source_->read(m_buffer_.use_data(), m_buffer_.get_buffer_size(), &input_bytes_read);
            if (YETI_FAILED(result)) return result;

            m_buffer_.set_data_size(input_bytes_read);
            m_state_->m_stream_.next_in = m_buffer_.use_data();
            m_state_->m_stream_.avail_in = m_buffer_.get_data_size();
        } else {
            return Zip::map_error(err);
        }
    }

    YETI_Size progress = bytes_to_read - m_state_->m_stream_.avail_out;
    if (bytes_read) *bytes_read = progress;
    m_position_ += progress;

    return progress == 0 ? YETI_ERROR_EOS : YETI_SUCCESS;
}

YETI_Result ZipInflatingInputStream::seek(YETI_Position /*offset*/)
{
    return YETI_ERROR_NOT_SUPPORTED;
}

YETI_Result ZipInflatingInputStream::tell(YETI_Position & offset)
{
    offset = m_position_;
    return YETI_SUCCESS;
}

YETI_Result ZipInflatingInputStream::get_size(YETI_LargeSize & size)
{
    size = 0;
    return YETI_ERROR_NOT_SUPPORTED;
}

YETI_Result ZipInflatingInputStream::get_available(YETI_LargeSize & available)
{
    available = 0;
    return YETI_SUCCESS;
}

ZipDeflatingInputStream::ZipDeflatingInputStream(InputStreamReference & source, int compression_level /* = YETI_ZIP_COMPRESSION_LEVEL_DEFAULT */, Zip::TypeFormat format /* = Zip::ZLIB */)
: m_source_(source)
, m_position_(0)
, m_eos_(false)
, m_state_(new ZipDeflateState(compression_level, format))
, m_buffer_(YETI_ZIP_DEFAULT_BUFFER_SIZE)
{
}

ZipDeflatingInputStream::~ZipDeflatingInputStream()
{
    delete m_state_;
    m_state_ = NULL;
}

YETI_Result ZipDeflatingInputStream::read(void * buffer,
                                          YETI_Size bytes_to_read,
                                          YETI_Size * bytes_read /*= NULL*/)
{
    if (m_state_ == NULL) return YETI_ERROR_INVALID_STATE;
    if (buffer == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    if (bytes_to_read == 0) return YETI_SUCCESS;

    if (bytes_read) *bytes_read = 0;

    m_state_->m_stream_.next_out = (Bytef *)buffer;
    m_state_->m_stream_.avail_out = (uInt)bytes_to_read;

    while (m_state_->m_stream_.avail_out) {
        int err = deflate(&m_state_->m_stream_, m_eos_ ? Z_FINISH : Z_NO_FLUSH);
        if (err == Z_STREAM_END) {
            break;
        } else if (err == Z_OK) {
            continue;
        } else if (err == Z_BUF_ERROR) {
            YETI_Size input_bytes_read = 0;
            YETI_Result result = m_source_->read(m_buffer_.use_data(), m_buffer_.get_buffer_size(), &input_bytes_read);
            if (result == YETI_ERROR_EOS) {
                m_eos_ = true; 
            } else {
                if (YETI_FAILED(result)) return result;
            }

            m_buffer_.set_data_size(input_bytes_read);
            m_state_->m_stream_.next_in = m_buffer_.use_data();
            m_state_->m_stream_.avail_in = m_buffer_.get_data_size();
        } else {
            return Zip::map_error(err);
        }
    }

    YETI_Size progress = bytes_to_read - m_state_->m_stream_.avail_out;
    if (bytes_read) *bytes_read = progress;
    m_position_ += progress;

    return progress == 0 ? YETI_ERROR_EOS : YETI_SUCCESS;
}

YETI_Result ZipDeflatingInputStream::seek(YETI_Position /*offset*/)
{
    return YETI_ERROR_NOT_SUPPORTED;
}

YETI_Result ZipDeflatingInputStream::tell(YETI_Position & offset)
{
    offset = m_position_;
    return YETI_SUCCESS;
}

YETI_Result ZipDeflatingInputStream::get_size(YETI_LargeSize & size)
{
    size = 0;
    return YETI_ERROR_NOT_SUPPORTED;
}

YETI_Result ZipDeflatingInputStream::get_available(YETI_LargeSize & available)
{
    available = 0;
    return YETI_SUCCESS;
}

YETI_Result Zip::compress_deflate(const DataBuffer & in, DataBuffer & out, int compression_level /* = YETI_ZIP_COMPRESSION_LEVEL_DEFAULT */, TypeFormat format /* = ZLIB */)
{
    out.set_data_size(0);

    if (compression_level < YETI_ZIP_COMPRESSION_LEVEL_DEFAULT ||
        compression_level > YETI_ZIP_COMPRESSION_LEVEL_MAX) {
            return YETI_ERROR_INVALID_PARAMETERS;
    }

    z_stream stream;
    SetMemory(&stream, 0, sizeof(stream));
    stream.next_in   = (Bytef*)in.get_data();
    stream.avail_in  = (uInt)in.get_data_size();

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    int err = deflateInit2(&stream, 
        compression_level,
        Z_DEFLATED,
        15 + (format == GZIP ? 16 : 0),
        8,
        Z_DEFAULT_STRATEGY);
    if (err != Z_OK) return map_error(err);

    out.reserve(deflateBound(&stream, stream.avail_in) + (format == GZIP ? 10: 0));
    stream.next_out  = out.use_data();
    stream.avail_out = out.get_buffer_size();

    err = deflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        deflateEnd(&stream);
        return map_error(err);
    }

    out.set_data_size(stream.total_out);

    err = deflateEnd(&stream);
    return map_error(err);
}

YETI_Result Zip::decompress_inflate(const DataBuffer & in, DataBuffer & out)
{
    YETI_CHECK_WARNING(out.reserve(32 + 2 * in.get_data_size()));
    z_stream stream;
    stream.next_in = (Bytef *)in.get_data();
    stream.avail_in = (uInt)in.get_data_size();
    stream.next_out = out.use_data();
    stream.avail_out = (uInt)out.get_buffer_size();

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    int err = inflateInit2(&stream, 15 + 32);
    if (err != Z_OK) return map_error(err);

    do {
        err = inflate(&stream, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END || err == Z_OK || err == Z_BUF_ERROR) {
            out.set_data_size(stream.total_out);
            if ((err == Z_OK &&stream.avail_out == 0) || err == Z_BUF_ERROR) {
                out.reserve(out.get_buffer_size() * 2);
                stream.next_out = out.use_data() + stream.total_out;
                stream.avail_out = out.get_buffer_size() - stream.total_out;
            }
        }
    } while (err == Z_OK);

    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        return map_error(err);
    }

    err = inflateEnd(&stream);
    return map_error(err);
}

YETI_Result Zip::compress_deflate(File &in, File & out, int compression_level /* = YETI_ZIP_COMPRESSION_LEVEL_DEFAULT */, TypeFormat format /* = GZIP */)
{
    if (compression_level < YETI_ZIP_COMPRESSION_LEVEL_DEFAULT ||
        compression_level > YETI_ZIP_COMPRESSION_LEVEL_MAX) {
            return YETI_ERROR_INVALID_PARAMETERS;
    }

    InputStreamReference input;
    YETI_CHECK(in.get_input_stream(input));
    OutputStreamReference output;
    YETI_CHECK(out.get_output_stream(output));

    ZipDeflatingInputStream deflating_stream(input, compression_level, format);
    return stream_to_stream_copy(deflating_stream, *output.as_pointer());
}

NAMEEND
