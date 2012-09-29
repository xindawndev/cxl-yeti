#include <stdarg.h>

#include "YetiLogging.h"
#include "YetiList.h"
#include "YetiStreams.h"
#include "YetiSocket.h"
#include "YetiUtil.h"
#include "YetiFile.h"
#include "YetiSystem.h"
#include "YetiConsole.h"
#include "YetiDebug.h"

NAMEBEG

class LogConsoleHandler : public LogHandler {
public:
    enum {
        OUTPUT_TO_CONSOLE = 1,
        OUTPUT_TO_DEBUG   = 2
    };
    static YETI_Result create(const char * logger_name, LogHandler *& handler);
    void log(const LogRecord & record);

private:
    YETI_UInt32 m_outputs_;
    bool        m_use_colors_;
    YETI_Flags  m_format_filter_;
};

class LogFileHandler : public LogHandler {
public:
    static YETI_Result create(const char * logger_name, LogHandler *& handler);
    void log(const LogRecord & record);

private:
    YETI_Result _open(bool append = true);

private:
    bool                    m_flush_;
    bool                    m_append_;
    String                  m_filename_;
    YETI_Flags              m_format_filter_;
    YETI_LargeSize          m_max_file_size_;
    OutputStreamReference   m_stream_;
};

class LogTcpHandler : public LogHandler {
public:
    static void format_record(const LogRecord & record, String & msg);
    static YETI_Result create(const char * logger_name, LogHandler *& handler);

    void log(const LogRecord & record);

private:
    YETI_Result _connect();

private:
    String                  m_host_;
    YETI_UInt16             m_port_;
    OutputStreamReference   m_stream_;
};

class LogUdpHandler : public LogHandler {
public:
    static YETI_Result create(const char * logger_name, LogHandler *& handler);

    void log(const LogRecord & record);

private:
    UdpSocket       m_socket_;
    SocketAddress   m_target_;
};

class LogNullHandler : public LogHandler {
public:
    static YETI_Result create(LogHandler *& handler);

    void log(const LogRecord & record);
};

class LogCustomHandler : public LogHandler {
public:
    static YETI_Result set_custom_handler_function(custom_handler_external_funciton function);
    static YETI_Result create(LogHandler *& handler);

    void log(const LogRecord & record);

private:
    static custom_handler_external_funciton s_external_function_;
};

#define YETI_LOG_HEAP_BUFFER_INCREMENT 4096
#define YETI_LOG_STACK_BUFFER_MAX_SIZE 512
#define YETI_LOG_HEAP_BUFFER_MAX_SIZE  65536

#if !defined(YETI_CONFIG_LOG_CONFIG_ENV)
#   define YETI_CONFIG_LOG_CONFIG_ENV "YETI_LOG_CONFIG"
#endif

#if !defined(YETI_CONFIG_DEFAULT_LOG_CONFIG_SOURCE)
#   define YETI_CONFIG_DEFAULT_LOG_CONFIG_SOURCE "file:yeti-logging.properties"
#endif

#if !defined(YETI_CONFIG_DEFAULT_LOG_LEVEL)
#   define YETI_CONFIG_DEFAULT_LOG_LEVEL YETI_LOG_LEVEL_OFF
#endif

#define YETI_LOG_ROOT_DEFAULT_HANDLER   "ConsoleHandler"

#if !defined(YETI_CONFIG_DEFAULT_LOG_FILE_HANDLER_FILENAME)
#   define YETI_CONFIG_DEFAULT_LOG_FILE_HANDLER_FILENAME "_yeti.log"
#endif

#define YETI_LOG_TCP_HANDLER_DEFAULT_PORT            7723
#define YETI_LOG_TCP_HANDLER_DEFAULT_CONNECT_TIMEOUT 5000 /* 5 seconds */

#define YETI_LOG_UDP_HANDLER_DEFAULT_PORT            7724

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__APPLE__)
#   define YETI_LOG_CONSOLE_HANDLER_DEFAULT_COLOR_MODE false
#else
#   define YETI_LOG_CONSOLE_HANDLER_DEFAULT_COLOR_MODE true
#endif

#define YETI_LOG_FILE_HANDLER_MIN_RECYCLE_SIZE   1000000

#define YETI_LOG_FORMAT_FILTER_NO_SOURCE         1
#define YETI_LOG_FORMAT_FILTER_NO_TIMESTAMP      2
#define YETI_LOG_FORMAT_FILTER_NO_FUNCTION_NAME  4
#define YETI_LOG_FORMAT_FILTER_NO_LOGGER_NAME    8
#define YETI_LOG_FORMAT_FILTER_NO_SOURCEPATH    16
#define YETI_LOG_FORMAT_FILTER_NO_THREAD_ID     32

static LogManager g_log_mamager_;

class LogManagerAutoDisabler
{
public:
    LogManagerAutoDisabler() : m_was_enabled_(g_log_mamager_.is_enabled()) {
        g_log_mamager_.set_enabled(false);
    }

    ~LogManagerAutoDisabler() {
        g_log_mamager_.set_enabled(m_was_enabled_);
    }

private:
    bool m_was_enabled_;
};

class LogManagerAutoLocker
{
public:
    LogManagerAutoLocker(LogManager & manager) : m_manager_(manager) {
        m_manager_.lock();
    }

    ~LogManagerAutoLocker() {
        m_manager_.unlock();
    }

private:
    LogManager & m_manager_;
};

#if !defined(YETI_CONFIG_HAVE_SYSTEM_LOG_CONFIG)
YETI_Result get_system_log_config(String& /*config*/)
{
    return YETI_ERROR_NOT_SUPPORTED;
}
#endif

YETI_Result LogHandler::create(
                               const char * logger_name,
                               const char * handler_name,
                               LogHandler *& handler)
{
    handler = NULL;
    if (       StringEqual(handler_name, "NullHandler")) {
        return LogNullHandler::create(handler);
    } else if (StringEqual(handler_name, "FileHandler")) {
        return LogFileHandler::create(logger_name, handler);
    } else if (StringEqual(handler_name, "ConsoleHandler")) {
        return LogConsoleHandler::create(logger_name, handler);
    } else if (StringEqual(handler_name, "TcpHandler")) {
        return LogTcpHandler::create(logger_name, handler);
    } else if (StringEqual(handler_name, "UdpHandler")) {
        return LogUdpHandler::create(logger_name, handler);
    } else if (StringEqual(handler_name, "CustomHandler")) {
        return LogCustomHandler::create(handler);
    }

    return YETI_ERROR_NO_SUCH_CLASS;
}

YETI_Result LogHandler::set_custom_handler_function(custom_handler_external_funciton function)
{
    return LogCustomHandler::set_custom_handler_function(function);
}

int Log::get_log_level(const char * name)
{
    if (       StringEqual(name, "FATAL")) {
        return YETI_LOG_LEVEL_FATAL;
    } else if (StringEqual(name, "SEVERE")) {
        return YETI_LOG_LEVEL_SEVERE;
    } else if (StringEqual(name, "WARNING")) {
        return YETI_LOG_LEVEL_WARNING;
    } else if (StringEqual(name, "INFO")) {
        return YETI_LOG_LEVEL_INFO;
    } else if (StringEqual(name, "FINE")) {
        return YETI_LOG_LEVEL_FINE;
    } else if (StringEqual(name, "FINER")) {
        return YETI_LOG_LEVEL_FINER;
    } else if (StringEqual(name, "FINEST")) {
        return YETI_LOG_LEVEL_FINEST;
    } else if (StringEqual(name, "ALL")) {
        return YETI_LOG_LEVEL_ALL;
    } else if (StringEqual(name, "OFF")) {
        return YETI_LOG_LEVEL_OFF;
    } else {
        return -1;
    }
}

const char * Log::get_log_level_name(int level)
{
    switch (level) {
case YETI_LOG_LEVEL_FATAL:   return "FATAL";
case YETI_LOG_LEVEL_SEVERE:  return "SEVERE";
case YETI_LOG_LEVEL_WARNING: return "WARNING";
case YETI_LOG_LEVEL_INFO:    return "INFO";
case YETI_LOG_LEVEL_FINE:    return "FINE";
case YETI_LOG_LEVEL_FINER:   return "FINER";
case YETI_LOG_LEVEL_FINEST:  return "FINEST";
case YETI_LOG_LEVEL_OFF:     return "OFF";
default:                     return "";
    }
}

const char * Log::get_log_level_asic_color(int level)
{
    switch (level) {
case YETI_LOG_LEVEL_FATAL:   return "31";
case YETI_LOG_LEVEL_SEVERE:  return "31";
case YETI_LOG_LEVEL_WARNING: return "33";
case YETI_LOG_LEVEL_INFO:    return "32";
case YETI_LOG_LEVEL_FINE:    return "34";
case YETI_LOG_LEVEL_FINER:   return "35";
case YETI_LOG_LEVEL_FINEST:  return "36";
default:                     return NULL;
    }
}

void Log::format_record_to_stream(
                                  const LogRecord & record,
                                  OutputStream & stream,
                                  bool use_colors,
                                  YETI_Flags format_filter)
{
    
}

NAMEEND
