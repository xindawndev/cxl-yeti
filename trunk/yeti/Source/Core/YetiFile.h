#ifndef _CXL_YETI_FILE_H_
#define _CXL_YETI_FILE_H_

#include "YetiTypes.h"
#include "YetiStreams.h"
#include "YetiTime.h"

NAMEBEG

const int YETI_ERROR_NO_SUCH_FILE          = YETI_ERROR_BASE_FILE - 0;
const int YETI_ERROR_FILE_NOT_OPEN         = YETI_ERROR_BASE_FILE - 1;
const int YETI_ERROR_FILE_BUSY             = YETI_ERROR_BASE_FILE - 2;
const int YETI_ERROR_FILE_ALREADY_OPEN     = YETI_ERROR_BASE_FILE - 3;
const int YETI_ERROR_FILE_NOT_READABLE     = YETI_ERROR_BASE_FILE - 4;
const int YETI_ERROR_FILE_NOT_WRITABLE     = YETI_ERROR_BASE_FILE - 5;
const int YETI_ERROR_FILE_NOT_DIRECTORY    = YETI_ERROR_BASE_FILE - 6;
const int YETI_ERROR_FILE_ALREADY_EXISTS   = YETI_ERROR_BASE_FILE - 7;
const int YETI_ERROR_FILE_NOT_ENOUGH_SPACE = YETI_ERROR_BASE_FILE - 8;
const int YETI_ERROR_DIRECTORY_NOT_EMPTY   = YETI_ERROR_BASE_FILE - 9;

const unsigned int YETI_FILE_OPEN_MODE_READ       = 0x01;
const unsigned int YETI_FILE_OPEN_MODE_WRITE      = 0x02;
const unsigned int YETI_FILE_OPEN_MODE_CREATE     = 0x04;
const unsigned int YETI_FILE_OPEN_MODE_TRUNCATE   = 0x08;
const unsigned int YETI_FILE_OPEN_MODE_UNBUFFERED = 0x10;
const unsigned int YETI_FILE_OPEN_MODE_APPEND     = 0x20;

const unsigned int YETI_FILE_ATTRIBUTE_READ_ONLY = 0x01;
const unsigned int YETI_FILE_ATTRIBUTE_LINK      = 0x02;

#define YETI_FILE_STANDARD_INPUT  "@STDIN"
#define YETI_FILE_STANDARD_OUTPUT "@STDOUT"
#define YETI_FILE_STANDARD_ERROR  "@STDERR"

class DataBuffer;

struct FileInfo 
{
    typedef enum {
        FILE_TYPE_NONE,
        FILE_TYPE_REGULAR,
        FILE_TYPE_DIRECTORY,
        FILE_TYPE_SPECIAL,
        FILE_TYPE_OTHER
    } FileType;

    FileInfo() : m_type_(FILE_TYPE_NONE), m_size_(0), m_attributes_mask_(0), m_attributes_(0) {}

    FileType m_type_;
    YETI_UInt64 m_size_;
    YETI_Flags m_attributes_mask_;
    YETI_Flags m_attributes_;
    TimeStamp m_creation_time_;
    TimeStamp m_modification_time_;
};

class FilePath
{
public:
    static const char * const separator;

    static String base_name(const char * path, bool with_extension = true);
    static String dir_name(const char * path);
    static String file_extension(const char * path);
    static String create(const char * directory, const char * base);

private:
    FilePath() {}
};

class FileInterface
{
public:
    typedef unsigned int open_mode;

    virtual ~FileInterface() {}

    virtual YETI_Result open(open_mode mode) = 0;
    virtual YETI_Result close() = 0;
    virtual YETI_Result get_input_stream(InputStreamReference & stream) = 0;
    virtual YETI_Result get_output_stream(OutputStreamReference & stream) = 0;
};

class File : public FileInterface
{
public:
    static YETI_Result get_roots(List<String> & roots);
    static YETI_Result get_size(const char * path, YETI_LargeSize & size);
    static YETI_Result get_info(const char * path, FileInfo * info = NULL);
    static bool exists(const char * path) { return YETI_SUCCEEDED(get_info(path)); }
    static YETI_Result remove(const char * path, bool recurse = false);
    static YETI_Result remove_file(const char * path);
    static YETI_Result remove_dir(const char * path);
    static YETI_Result remove_dir(const char * path, bool force_if_not_empty);
    static YETI_Result rename(const char * from_path, const char * to_path);
    static YETI_Result list_dir(const char * path, List<String> & entries, YETI_Ordinal start = 0, YETI_Cardinal count = 0);
    static YETI_Result create_dir(const char * path);
    static YETI_Result create_dir(const char * path, bool creaate_intermediate_dirs);
    static YETI_Result get_working_dir(String & path);
    static YETI_Result load(const char * path, DataBuffer & buffer, FileInterface::open_mode mode = YETI_FILE_OPEN_MODE_READ);
    static YETI_Result load(const char * path, String & data, FileInterface::open_mode mode = YETI_FILE_OPEN_MODE_READ);;
    static YETI_Result save(const char * path, String & data);
    static YETI_Result save(const char * path, const DataBuffer & buffer);

    File(const char * path);
    ~File() { delete m_delegate_; }

    YETI_Result load(DataBuffer & buffer);
    YETI_Result save(const DataBuffer & buffer);
    const String & get_path() { return m_path_; }
    YETI_Result get_size(YETI_LargeSize & size);
    YETI_Result get_info(FileInfo & info);
    YETI_Result list_dir(List<String> & entries);
    YETI_Result rename(const char * path);

    virtual YETI_Result open(open_mode mode) {
        return m_delegate_->open(mode);
    }

    virtual YETI_Result close() {
        return m_delegate_->close();
    }

    virtual YETI_Result get_input_stream(InputStreamReference & stream) {
        return m_delegate_->get_input_stream(stream);
    }

    virtual YETI_Result get_output_stream(OutputStreamReference & stream) {
        return m_delegate_->get_output_stream(stream);
    }

    File & operator=(const File & file);

protected:
    FileInterface * m_delegate_;
    String m_path_;
    bool m_is_special_;
};

class FileDateComparator
{
public:
    FileDateComparator(const char * directory) : m_directory_(directory) {}
    YETI_Int32 operator()(const String & file1, const String & file2) const {
        FileInfo info1, info2;
        if (YETI_FAILED(File::get_info(FilePath::create(m_directory_, file1), &info1))) return -1;
        if (YETI_FAILED(File::get_info(FilePath::create(m_directory_, file2), &info2))) return -1;
        return (info1.m_modification_time_ == info2.m_modification_time_) ? 0 : (info1.m_modification_time_ < info2.m_modification_time_ ? -1 : 1);
    }
private:
    String m_directory_;
};


NAMEEND

#endif // _CXL_YETI_FILE_H_
