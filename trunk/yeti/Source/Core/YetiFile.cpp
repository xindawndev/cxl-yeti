#include "YetiFile.h"
#include "YetiUtil.h"
#include "YetiConstants.h"
#include "YetiStreams.h"
#include "YetiDataBuffer.h"
#include "YetiLogging.h"

YETI_SET_LOCAL_LOGGER("yeti.file")

NAMEBEG

String FilePath::base_name(const char * path, bool with_extension /* = true */)
{
    String result = path;
    int sep = result.reverse_find(separator);
    if (sep >= 0) {
        result = path + sep + StringLength(separator);
    }

    if (!with_extension) {
        int dot = result.reverse_find('.');
        if (dot >= 0) {
            result.set_length(dot);
        }
    }
    return result;
}

String FilePath::dir_name(const char * path)
{
    String result = path;
    int sep = result.reverse_find(separator);
    if (sep >= 0) {
        if (sep == 0) {
            result.set_length(StringLength(separator));
        } else {
            result.set_length(sep);
        }
    } else {
        result.set_length(0);
    }

    return result;
}

String FilePath::file_extension(const char * path)
{
    String result = path;
    int sep = result.reverse_find('.');
    if (sep >= 0) {
        result = path + sep;
    } else {
        result.set_length(0);
    }
    return result;
}

String FilePath::create(const char * directory, const char * base)
{
    if (!directory || StringLength(directory) == 0) return base;
    if (!base || StringLength(base) == 0) return directory;

    String result = directory;
    if (!result.ends_with(separator) && base[0] != separator[0]) {
        result += separator;
    }
    result += base;
    return result;
}

YETI_Result File::create_dir(const char * path, bool creaate_intermediate_dirs)
{
    String full_path = path;

    full_path.replace((FilePath::separator[0] == '/') ? '\\' : '/', FilePath::separator);
    full_path.trim_right(FilePath::separator);

    if (creaate_intermediate_dirs) {
        String dir_path;
        int sep = full_path.find(FilePath::separator, 1);
        while (sep > 0) {
            dir_path = full_path.sub_string(0, sep);
            YETI_CHECK_WARNING(File::create_dir(dir_path, false));
            sep = full_path.find(FilePath::separator, sep + 1);
        }
    }

    YETI_Result result = File::create_dir(full_path);

    if (YETI_FAILED(result) && result != YETI_ERROR_FILE_ALREADY_EXISTS) {
        return result;
    }

    return YETI_SUCCESS;
}

YETI_Result File::remove_dir(const char * path, bool force_if_not_empty)
{
    String root_path = path;

    root_path.replace((FilePath::separator[0] == '/') ? '\\' : '/', FilePath::separator);
    root_path.trim_right(FilePath::separator);

    if (force_if_not_empty) {
        File dir(root_path);
        List<String> entries;
        YETI_CHECK_WARNING(dir.list_dir(entries));
        for (List<String>::iterator it = entries.get_first_item(); it; ++it) {
            File::remove(FilePath::create(root_path, *it), true);
        }
    }

    return File::remove_dir(root_path);
}

YETI_Result File::load(const char * path, DataBuffer & buffer, FileInterface::open_mode mode /* = YETI_FILE_OPEN_MODE_READ */)
{
    File file(path);
    YETI_Result result = file.open(mode);
    if (YETI_FAILED(result)) return result;

    result = file.load(buffer);

    file.close();

    return result;
}

YETI_Result File::load(const char * path, String & data, FileInterface::open_mode mode /* = YETI_FILE_OPEN_MODE_READ */)
{
    DataBuffer buffer;

    data = "";

    File file(path);
    YETI_Result result = file.open(mode);
    if (YETI_FAILED(result)) return result;

    result = file.load(buffer);

    if (YETI_SUCCEEDED(result) && buffer.get_data_size() > 0) {
        data.assign((const char *)buffer.get_data(), buffer.get_data_size());
        data.set_length(buffer.get_data_size());
    }

    file.close();

    return result;
}

YETI_Result File::save(const char * path, String & data)
{
    DataBuffer buffer(data.get_chars(), data.get_length());
    return File::save(path, buffer);
}

YETI_Result File::save(const char * path, const DataBuffer & buffer)
{
    File file(path);
    YETI_Result result = file.open(YETI_FILE_OPEN_MODE_WRITE | YETI_FILE_OPEN_MODE_CREATE | YETI_FILE_OPEN_MODE_TRUNCATE);
    if (YETI_FAILED(result)) return result;

    result = file.save(buffer);

    file.close();

    return result;
}

YETI_Result File::load(DataBuffer & buffer)
{
    InputStreamReference input;
    YETI_CHECK_WARNING(get_input_stream(input));

    return input->load(buffer);
}

YETI_Result File::save(const DataBuffer & buffer)
{
    OutputStreamReference output;
    YETI_CHECK_WARNING(get_output_stream(output));
    return output->write_fully(buffer.get_data(), buffer.get_data_size());
}

YETI_Result File::get_info(FileInfo & info)
{
    if (m_is_special_) {
        info.m_type_ = FileInfo::FILE_TYPE_SPECIAL;
        info.m_size_ = 0;
        info.m_attributes_ = 0;
        info.m_attributes_mask_ = 0;
        return YETI_SUCCESS;
    }
    return get_info(m_path_.get_chars(), &info);
}

YETI_Result File::get_size(YETI_LargeSize & size)
{
    size = 0;

    FileInfo info;
    get_info(info);
    switch (info.m_type_) {
        case FileInfo::FILE_TYPE_DIRECTORY: {
            List<String> entries;
            YETI_CHECK_WARNING(list_dir(entries));
            size = entries.get_item_count();
            break;
                                            }
        case FileInfo::FILE_TYPE_REGULAR:
        case FileInfo::FILE_TYPE_OTHER:
            size = info.m_size_;
            return YETI_SUCCESS;
        default:
            break;
    }

    return YETI_SUCCESS;
}

YETI_Result File::get_size(const char * path, YETI_LargeSize & size)
{
    File file(path);
    return file.get_size(size);
}

YETI_Result File::remove(const char * path, bool recurse /* = false */)
{
    FileInfo info;
    YETI_CHECK_WARNING(get_info(path, &info));
    if (info.m_type_ == FileInfo::FILE_TYPE_DIRECTORY) {
        return remove_dir(path, recurse);
    }
    return remove_file(path);
}

YETI_Result File::rename(const char * path)
{
    YETI_Result result = rename(m_path_.get_chars(), path);
    if (YETI_SUCCEEDED(result)) {
        m_path_ = path;
    }
    return result;
}

YETI_Result File::list_dir(List<String> & entries)
{
    entries.clear();
    return list_dir(m_path_.get_chars(), entries);
}

NAMEEND

