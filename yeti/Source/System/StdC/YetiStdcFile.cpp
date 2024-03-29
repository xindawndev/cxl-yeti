#define _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE64
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#if !defined(_WIN32_WCE)
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#else
#include <stdio.h>
#define errno GetLastError()
#endif

#include "YetiConfig.h"
#include "YetiUtil.h"
#include "YetiFile.h"
#include "YetiThreads.h"
#include "YetiInterfaces.h"
#include "YetiString.h"
#include "YetiDebug.h"

#if defined(YETI_CONFIG_HAVE_SHARE_H)
#include <share.h>
#endif

#if defined(_WIN32)
extern FILE * yeti_fsopen_utf8(const char * path, const char * mode, int sh_flags);
extern FILE * yeti_fopen_utf8(const char * path, const char * mode);
#define fopen yeti_fopen_utf8
#define fopen_s yeti_fopen_s_utf8
#define  _fsopen yeti_fsopen_utf8
#endif

#if !defined(YETI_CONFIG_HAVE_FOPEN_S)
static int fopen_s(FILE**      file,
                   const char* filename,
                   const char* mode)
{
    *file = fopen(filename, mode);

#if defined(_WIN32_WCE)
    if (*file == NULL) return ENOENT;
#else
    if (*file == NULL) return errno;
#endif
    return 0;
}
#endif // defined(YETI_CONFIG_HAVE_FOPEN_S

static YETI_Result map_errno(int err)
{
    USINGNAMESPACE2;
    switch (err) {
      case EACCES:       return YETI_ERROR_PERMISSION_DENIED;
      case EPERM:        return YETI_ERROR_PERMISSION_DENIED;
      case ENOENT:       return YETI_ERROR_NO_SUCH_FILE;
#if defined(ENAMETOOLONG)
      case ENAMETOOLONG: return YETI_ERROR_INVALID_PARAMETERS;
#endif
      case EBUSY:        return YETI_ERROR_FILE_BUSY;
      case EROFS:        return YETI_ERROR_FILE_NOT_WRITABLE;
      case ENOTDIR:      return YETI_ERROR_FILE_NOT_DIRECTORY;
      default:           return YETI_ERROR_ERRNO(err);
    }
}

NAMEBEG

class StdcFileWrapper
{
public:
    StdcFileWrapper(FILE * file, const char * name) : m_file_(file), m_name_(name) {}
    ~StdcFileWrapper() {
        if (m_file_ != NULL &&
            m_file_ != stdin &&
            m_file_ != stdout &&
            m_file_ != stderr) {
                fclose(m_file_);
        }
    }

    FILE * m_file_;
    String m_name_;
};

typedef Reference<StdcFileWrapper> StdcFileReference;

class StdcFileStream
{
public:
    StdcFileStream(StdcFileReference file)
        : m_file_reference_(file) {}

    YETI_Result seek(YETI_Position offset);
    YETI_Result tell(YETI_Position & offset);
    YETI_Result flush();

protected:
    virtual ~StdcFileStream() {}
    StdcFileReference m_file_reference_;
};

YETI_Result StdcFileStream::seek(YETI_Position offset)
{
    size_t result = YETI_fseek(m_file_reference_->m_file_, offset, SEEK_SET);
    if (result == 0) {
        return YETI_SUCCESS;
    }
    return YETI_FAILURE;
}

YETI_Result StdcFileStream::tell(YETI_Position & offset)
{
    offset = 0;
    YETI_Int64 pos = YETI_ftell(m_file_reference_->m_file_);
    if (pos < 0) return YETI_FAILURE;
    offset = pos;
    return YETI_SUCCESS;
}

YETI_Result StdcFileStream::flush()
{
    fflush(m_file_reference_->m_file_);
    return YETI_SUCCESS;
}

class StdcFileInputStream : public InputStream, private StdcFileStream
{
public:
    StdcFileInputStream(StdcFileReference & file)
        : StdcFileStream(file) {}

    virtual YETI_Result read(void * buffer,
        YETI_Size bytes_to_read,
        YETI_Size * bytes_read);
    virtual YETI_Result seek(YETI_Position offset) {
        return StdcFileStream::seek(offset);
    }
    virtual YETI_Result tell(YETI_Position & offset) {
        return StdcFileStream::tell(offset);
    }
    virtual YETI_Result get_size(YETI_LargeSize & size);
    virtual YETI_Result get_available(YETI_LargeSize & available);
};

YETI_Result StdcFileInputStream::read(void * buffer, YETI_Size bytes_to_read, YETI_Size * bytes_read)
{
    size_t nb_read;
    if (buffer == NULL) {
        return YETI_ERROR_INVALID_PARAMETERS;
    }

    nb_read = fread(buffer, 1, bytes_to_read, m_file_reference_->m_file_);
    if (nb_read > 0) {
        if (bytes_read) *bytes_read = (YETI_Size)nb_read;
        return YETI_SUCCESS;
    } else if (feof(m_file_reference_->m_file_)) {
        if (bytes_read) *bytes_read = 0;
        return YETI_ERROR_EOS;
    } else {
        if (bytes_read) *bytes_read = 0;
        return map_errno(errno);
    }
}

YETI_Result StdcFileInputStream::get_size(YETI_LargeSize & size)
{
    FileInfo file_info;
    YETI_Result result = File::get_info(m_file_reference_->m_name_, &file_info);
    if (YETI_FAILED(result)) return result;
    size = file_info.m_size_;
    return YETI_SUCCESS;
}

YETI_Result StdcFileInputStream::get_available(YETI_LargeSize & available)
{
    YETI_Int64 offset = YETI_ftell(m_file_reference_->m_file_);
    YETI_LargeSize size = 0;

    if (YETI_SUCCEEDED(get_size(size) && offset >= 0 && (YETI_LargeSize)offset <= size)) {
        available = size - offset;
        return YETI_SUCCESS;
    } else {
        available = 0;
        return YETI_FAILURE;
    }
}

class StdcFileOutputStream : public OutputStream, private StdcFileStream
{
public:StdcFileOutputStream(StdcFileReference & file)
           : StdcFileStream(file) {}

       YETI_Result write(const void * buffer,
           YETI_Size bytes_to_write,
           YETI_Size * bytes_written);
       YETI_Result seek(YETI_Position offset) {
           return StdcFileStream::seek(offset);
       }
       YETI_Result tell(YETI_Position & offset) {
           return StdcFileStream::tell(offset);
       }
       YETI_Result flush() {
           return StdcFileStream::flush();
       }
};

YETI_Result StdcFileOutputStream::write(const void * buffer, YETI_Size bytes_to_write, YETI_Size * bytes_written)
{
    size_t nb_written;
    nb_written = fwrite(buffer, 1, bytes_to_write, m_file_reference_->m_file_);
    if (nb_written > 0) {
        if (bytes_written) * bytes_written = (YETI_Size)nb_written;
        return YETI_SUCCESS;
    } else {
        if (bytes_written) *bytes_written = 0;
        return YETI_ERROR_WRITE_FAILED;
    }
}

class StdcFile : public FileInterface
{
public:
    StdcFile(File & delegator)
        : m_delegator_(delegator)
        , m_mode_(0) {}
    ~StdcFile() {
        close();
    }

    YETI_Result open(open_mode mode);
    YETI_Result close();
    YETI_Result get_input_stream(InputStreamReference & stream);
    YETI_Result get_output_stream(OutputStreamReference & stream);

private:
    File & m_delegator_;
    open_mode m_mode_;
    StdcFileReference m_file_reference_;
};

YETI_Result StdcFile::open(open_mode mode)
{
    FILE * file = NULL;
    if (!m_file_reference_.is_null()) {
        return YETI_ERROR_FILE_ALREADY_OPEN;
    }
    m_mode_ = mode;
    const char * name = (const char *)m_delegator_.get_path();
    if (StringEqual(name, YETI_FILE_STANDARD_INPUT)) {
        file = stdin;
    } else if (StringEqual(name, YETI_FILE_STANDARD_OUTPUT)) {
        file = stdout;
    } else if (StringEqual(name, YETI_FILE_STANDARD_ERROR)) {
        file = stderr;
    } else {
        const char * fmode = "";
        if (mode & YETI_FILE_OPEN_MODE_WRITE) {
            if (mode & YETI_FILE_OPEN_MODE_WRITE) {
                fmode = "a+b";
            } else {
                if (mode & YETI_FILE_OPEN_MODE_CREATE || (mode & YETI_FILE_OPEN_MODE_TRUNCATE)) {
                    fmode = "w+b";
                } else {
                    fmode = "r+b";
                }
            }
        } else {
            fmode = "rb";
        }

#if defined(YETI_CONFIG_HAVE_FSOPEN)
        file = _fsopen(name, fmode, _SH_DENYNO);
        int open_result = file == NULL ? ENOENT : 0;
#else
        int open_result = fopen_s(&file, name, fmode);
#endif
        if (open_result != 0) return map_errno(open_result);
    }

    if ((mode & YETI_FILE_OPEN_MODE_UNBUFFERED) && file != NULL) {
#if !defined(_WIN32_WCE)
        setvbuf(file, NULL, _IONBF, 0);
#endif
    }
    m_file_reference_ = new StdcFileWrapper(file, name);
    return YETI_SUCCESS;
}

YETI_Result StdcFile::close()
{
    m_file_reference_ = NULL;
    m_mode_ = 0;
    return YETI_SUCCESS;
}

YETI_Result StdcFile::get_input_stream(InputStreamReference & stream)
{
    stream = NULL;
    if (m_file_reference_.is_null()) return YETI_ERROR_FILE_NOT_OPEN;
    if (!(m_mode_ & YETI_FILE_OPEN_MODE_READ)) {
        return YETI_ERROR_FILE_NOT_READABLE;
    }
    stream = new StdcFileInputStream(m_file_reference_);
    return YETI_SUCCESS;
}

YETI_Result StdcFile::get_output_stream(OutputStreamReference & stream)
{
    stream = NULL;
    if (m_file_reference_.is_null()) return YETI_ERROR_FILE_NOT_OPEN;
    if (!(m_mode_ & YETI_FILE_OPEN_MODE_WRITE)) {
        return YETI_ERROR_FILE_NOT_WRITABLE;
    }
    stream = new StdcFileOutputStream(m_file_reference_);
    return YETI_SUCCESS;
}

File::File(const char * path)
{
    m_delegate_ = new StdcFile(*this);
    if (StringEqual(path, YETI_FILE_STANDARD_INPUT) ||
        StringEqual(path, YETI_FILE_STANDARD_OUTPUT) ||
        StringEqual(path, YETI_FILE_STANDARD_ERROR)) {
        m_is_special_ = true;
    }
}

File & File::operator =(const File & file)
{
    if (this != &file) {
        delete m_delegate_;
        m_path_ = file.m_path_;
        m_is_special_ = file.m_is_special_;
        m_delegate_ = new StdcFile(*this);
    }
    return *this;
}

NAMEEND
