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

NAMEEND