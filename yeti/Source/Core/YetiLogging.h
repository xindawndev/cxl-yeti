#ifndef _CXL_YETI_LOGGING_H_
#define _CXL_YETI_LOGGING_H_

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiTime.h"
#include "YetiString.h"
#include "YetiList.h"
#include "YetiStreams.h"
#include "YetiThreads.h"
#include "YetiHttp.h"

NAMEBEG

class LogManager;

class LogRecord {
public:
    const char *    m_logger_name_;
    int             m_level_;
    const char *    m_message_;
    TimeStamp       m_timestamp_;
    const char *    m_source_file_;
    unsigned int    m_source_line_;
    const char *    m_source_function_;
    unsigned long   m_thread_id_;
};

class LogHandler
{
public:
    typedef void (* custom_handler_external_funciton)(const LogRecord * record);

    static YETI_Result set_custom_handler_function(custom_handler_external_funciton function);
    static YETI_Result create(const char * logger_name,
        const char * handler_name,
        LogHandler *& handler);

    virtual ~LogHandler() {}
    virtual void log(const LogRecord & record) = 0;
    virtual String to_string() { return ""; }
};

class Logger
{
public:
    Logger(const char * name, LogManager & manager);
    ~Logger();

    void log(int level,
        const char * source_file,
        unsigned int source_line,
        const char * source_function,
        const char * msg, ...);

    YETI_Result             add_handler(LogHandler * handler, bool transfer_ownership = true);
    YETI_Result             delete_handlers();
    YETI_Result             set_parent(Logger * parent);
    const String &          get_name() const { return m_name_; }
    int                     get_level() const { return m_level_; }
    bool                    get_forward_to_parent() const { return m_forward_to_parent_; }
    List<LogHandler *> &    get_handlers() { return m_handlers_; }

private:
    LogManager &        m_manager_;
    String              m_name_;
    int                 m_level_;
    bool                m_level_is_inherited;
    bool                m_forward_to_parent_;
    Logger *            m_parent_;
    List<LogHandler *>  m_handlers_;
    List<LogHandler *>  m_external_handlers_;

    friend class LogManager;
};

typedef struct {
    Logger * logger;
    const char * name;
} LoggerReference;

class Log
{
public:
    static int get_log_level(const char * name);
    static const char * get_log_level_name(int level);
    static const char * get_log_level_asic_color(int level);
    static void format_record_to_stream(const LogRecord & record,
        OutputStream & stream,
        bool use_colors,
        YETI_Flags format_filter);
};

class LogConfigEntry{
public:
    LogConfigEntry(const char * key, const char * value) :
    m_key_(key), m_value_(value) {}
    String m_key_;
    String m_value_;
};

class LogManager
{
public:
    static LogManager & get_default();
    static bool config_value_is_boolean_true(String & value);
    static bool config_value_is_boolean_false(String & value);
    static Logger * get_logger(const char * name);

    LogManager();
    ~LogManager();
    
    YETI_Result             configure(const char * config_sources = NULL);
    String *                get_config_value(const char * prefix, const char * suffix);
    List<Logger *> &        get_loggers() { return m_loggers_; }
    List<LogConfigEntry> &  get_config() { return m_config_; }
    void                    set_enabled(bool enabled) { m_enable_ = enabled; }
    bool                    is_enabled() { return m_enable_; }
    void                    lock();
    void                    unlock();

private:
    YETI_Result _set_config_value(const char * key, const char * value);
    YETI_Result _parse_config(const char * config, YETI_Size config_size);
    YETI_Result _parse_config_source(String & source);
    YETI_Result _parse_config_file(const char * filename);
    bool        _have_logger_config(const char * name);
    Logger *    _find_logger(const char * name);
    YETI_Result _configure_logger(Logger * logger);

    Mutex                   m_lock_;
    Thread::threadid        m_lock_owner_;
    bool                    m_enable_;
    bool                    m_configured_;
    List<LogConfigEntry>    m_config_;
    List<Logger *>          m_loggers_;
    Logger *                m_root_;
};

const unsigned short YETI_HTTP_LOGGER_CONFIGURATOR_DEFAULT_PORT = 6378;
class HttpLoggerConfigurator : HttpRequestHandler, public Thread
{
public:
    HttpLoggerConfigurator(YETI_UInt16 port = YETI_HTTP_LOGGER_CONFIGURATOR_DEFAULT_PORT, bool detached = true);
    virtual ~HttpLoggerConfigurator();

    virtual void run();

private:
    virtual YETI_Result setup_response(HttpRequest & request,
        const HttpRequestContext & context,
        HttpResponse & respons);

    HttpServer * m_server_;
};

YETI_Result get_system_log_config(String & config);

#define YETI_LOG_LEVEL_FATAL   700
#define YETI_LOG_LEVEL_SEVERE  600 
#define YETI_LOG_LEVEL_WARNING 500
#define YETI_LOG_LEVEL_INFO    400
#define YETI_LOG_LEVEL_FINE    300
#define YETI_LOG_LEVEL_FINER   200
#define YETI_LOG_LEVEL_FINEST  100 

#define YETI_LOG_LEVEL_OFF     32767
#define YETI_LOG_LEVEL_ALL     0

#define YETI_LOG_GET_LOGGER(_logger)                                \
    if ((_logger).logger == NULL) {                                 \
    (_logger).logger = YETI_LogManager::GetLogger((_logger).name);  \
    }

#if defined(YETI_CONFIG_ENABLE_LOGGING)
//TODO: volatile makes tons of errors for me
//#define YETI_DEFINE_LOGGER(_logger, _name) static volatile YETI_LoggerReference _logger = { NULL, (_name) };
#define YETI_DEFINE_LOGGER(_logger, _name) static YETI_LoggerReference _logger = { NULL, (_name) };

#define YETI_LOG_X(_logger, _level, _argsx)                              \
    do {                                                                    \
    YETI_LOG_GET_LOGGER((_logger))                                       \
    if ((_logger).logger && (_level) >= (_logger).logger->GetLevel()) { \
    (_logger).logger->Log _argsx;                                   \
    }                                                                   \
    } while(0)

#define YETI_CHECK_LL(_logger, _level, _result) do {                                    \
    YETI_Result _x = (_result);                                                         \
    if (_x != YETI_SUCCESS) {                                                           \
    YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),"YETI_CHECK failed, result=%d (%s) [%s]", _x, error_message(_x), #_result)); \
    return _x;                                                                     \
    }                                                                                  \
} while(0)

#define YETI_CHECK_LABEL_LL(_logger, _level, _result, _label) do {                      \
    YETI_Result _x = (_result);                                                         \
    if (_x != YETI_SUCCESS) {                                                           \
    YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),"YETI_CHECK failed, result=%d (%s) [%s]", _x, error_message(_x), #_result)); \
    goto _label;                                                                   \
    }                                                                                  \
} while(0)
#define YETI_CHECK_POINTER_LL(_logger, _level, _p) do {                                 \
    if ((_p) == NULL) {                                                                  \
    YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),"@@@ NULL pointer parameter"));                     \
    return YETI_ERROR_INVALID_PARAMETERS;                                                     \
    }                                                                                  \
} while(0)
#define YETI_CHECK_POINTER_LABEL_LL(_logger, _level, _p, _label) do {                   \
    if ((_p) == NULL) {                                                                  \
    YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),"@@@ NULL pointer parameter"));                     \
    goto _label;                                                                   \
    }                                                                                  \
} while(0)

#else /* YETI_CONFIG_ENABLE_LOGGING */

#define YETI_DEFINE_LOGGER(_logger, _name)
#define YETI_LOG_X(_logger, _level, _argsx)
#define YETI_CHECK_LL(_logger, _level, _result) YETI_CHECK(_result)
#define YETI_CHECK_LABEL_LL(_logger, _level, _result, _label) YETI_CHECK_LABEL((_result), _label)
#define YETI_CHECK_POINTER_LL(_logger, _level, _p) YETI_CHECK_POINTER((_p))
#define YETI_CHECK_POINTER_LABEL_LL(_logger, _level, _p, _label) YETI_CHECK_POINTER_LABEL((_p), _label)

#endif /* YETI_CONFIG_ENABLE_LOGGING */

#define YETI_SET_LOCAL_LOGGER(_name) YETI_DEFINE_LOGGER(_YETI_LocalLogger, (_name))
#define YETI_CHECK_L(_level, _result) YETI_CHECK_LL(_YETI_LocalLogger, (_level), (_result))
#define YETI_CHECK_LABEL_L(_level, _result, _label) YETI_CHECK_LABEL_LL(_YETI_LocalLogger, (_level), NULL, (_result), _label)

/* NOTE: the following are machine-generated, do not edit */
#define YETI_LOG_LL(_logger,_level,_msg) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg)))
#define YETI_LOG(_level,_msg) YETI_LOG_LL((_YETI_LocalLogger),(_level),(_msg))
#define YETI_LOG_L(_logger,_level,_msg) YETI_LOG_LL((_logger),(_level),(_msg))
#define YETI_LOG_LL1(_logger,_level,_msg,_arg1) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1)))
#define YETI_LOG_1(_level,_msg,_arg1) YETI_LOG_LL1((_YETI_LocalLogger),(_level),(_msg),(_arg1))
#define YETI_LOG_L1(_logger,_level,_msg,_arg1) YETI_LOG_LL1((_logger),(_level),(_msg),(_arg1))
#define YETI_LOG_LL2(_logger,_level,_msg,_arg1,_arg2) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1),(_arg2)))
#define YETI_LOG_2(_level,_msg,_arg1,_arg2) YETI_LOG_LL2((_YETI_LocalLogger),(_level),(_msg),(_arg1),(_arg2))
#define YETI_LOG_L2(_logger,_level,_msg,_arg1,_arg2) YETI_LOG_LL2((_logger),(_level),(_msg),(_arg1),(_arg2))
#define YETI_LOG_LL3(_logger,_level,_msg,_arg1,_arg2,_arg3) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1),(_arg2),(_arg3)))
#define YETI_LOG_3(_level,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_YETI_LocalLogger),(_level),(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_L3(_logger,_level,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_logger),(_level),(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_LL4(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1),(_arg2),(_arg3),(_arg4)))
#define YETI_LOG_4(_level,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_YETI_LocalLogger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_L4(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_logger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_LL5(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5)))
#define YETI_LOG_5(_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_YETI_LocalLogger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_L5(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_logger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_LL6(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6)))
#define YETI_LOG_6(_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_YETI_LocalLogger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_L6(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_logger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_LL7(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7)))
#define YETI_LOG_7(_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_YETI_LocalLogger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_L7(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_logger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_LL8(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8)))
#define YETI_LOG_8(_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_YETI_LocalLogger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_L8(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_logger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_LL9(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_X((_logger),(_level),((_level),__FILE__,__LINE__,(YETI_LocalFunctionName),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9)))
#define YETI_LOG_9(_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_YETI_LocalLogger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_L9(_logger,_level,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_logger),(_level),(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))

#define YETI_LOG_FATAL(_msg) YETI_LOG_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg))
#define YETI_LOG_FATAL_L(_logger,_msg) YETI_LOG_LL((_logger),YETI_LOG_LEVEL_FATAL,(_msg))
#define YETI_LOG_FATAL_1(_msg,_arg1) YETI_LOG_LL1((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1))
#define YETI_LOG_FATAL_L1(_logger,_msg,_arg1) YETI_LOG_LL1((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1))
#define YETI_LOG_FATAL_2(_msg,_arg1,_arg2) YETI_LOG_LL2((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2))
#define YETI_LOG_FATAL_L2(_logger,_msg,_arg1,_arg2) YETI_LOG_LL2((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2))
#define YETI_LOG_FATAL_3(_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_FATAL_L3(_logger,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_FATAL_4(_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_FATAL_L4(_logger,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_FATAL_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_FATAL_L5(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_FATAL_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_FATAL_L6(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_FATAL_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_FATAL_L7(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_FATAL_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_FATAL_L8(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_FATAL_9(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_FATAL_L9(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_logger),YETI_LOG_LEVEL_FATAL,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_SEVERE(_msg) YETI_LOG_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg))
#define YETI_LOG_SEVERE_L(_logger,_msg) YETI_LOG_LL((_logger),YETI_LOG_LEVEL_SEVERE,(_msg))
#define YETI_LOG_SEVERE_1(_msg,_arg1) YETI_LOG_LL1((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1))
#define YETI_LOG_SEVERE_L1(_logger,_msg,_arg1) YETI_LOG_LL1((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1))
#define YETI_LOG_SEVERE_2(_msg,_arg1,_arg2) YETI_LOG_LL2((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2))
#define YETI_LOG_SEVERE_L2(_logger,_msg,_arg1,_arg2) YETI_LOG_LL2((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2))
#define YETI_LOG_SEVERE_3(_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_SEVERE_L3(_logger,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_SEVERE_4(_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_SEVERE_L4(_logger,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_SEVERE_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_SEVERE_L5(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_SEVERE_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_SEVERE_L6(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_SEVERE_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_SEVERE_L7(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_SEVERE_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_SEVERE_L8(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_SEVERE_9(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_SEVERE_L9(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_logger),YETI_LOG_LEVEL_SEVERE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_WARNING(_msg) YETI_LOG_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg))
#define YETI_LOG_WARNING_L(_logger,_msg) YETI_LOG_LL((_logger),YETI_LOG_LEVEL_WARNING,(_msg))
#define YETI_LOG_WARNING_1(_msg,_arg1) YETI_LOG_LL1((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1))
#define YETI_LOG_WARNING_L1(_logger,_msg,_arg1) YETI_LOG_LL1((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1))
#define YETI_LOG_WARNING_2(_msg,_arg1,_arg2) YETI_LOG_LL2((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2))
#define YETI_LOG_WARNING_L2(_logger,_msg,_arg1,_arg2) YETI_LOG_LL2((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2))
#define YETI_LOG_WARNING_3(_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_WARNING_L3(_logger,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_WARNING_4(_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_WARNING_L4(_logger,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_WARNING_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_WARNING_L5(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_WARNING_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_WARNING_L6(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_WARNING_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_WARNING_L7(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_WARNING_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_WARNING_L8(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_WARNING_9(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_WARNING_L9(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_logger),YETI_LOG_LEVEL_WARNING,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_INFO(_msg) YETI_LOG_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg))
#define YETI_LOG_INFO_L(_logger,_msg) YETI_LOG_LL((_logger),YETI_LOG_LEVEL_INFO,(_msg))
#define YETI_LOG_INFO_1(_msg,_arg1) YETI_LOG_LL1((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1))
#define YETI_LOG_INFO_L1(_logger,_msg,_arg1) YETI_LOG_LL1((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1))
#define YETI_LOG_INFO_2(_msg,_arg1,_arg2) YETI_LOG_LL2((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2))
#define YETI_LOG_INFO_L2(_logger,_msg,_arg1,_arg2) YETI_LOG_LL2((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2))
#define YETI_LOG_INFO_3(_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_INFO_L3(_logger,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_INFO_4(_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_INFO_L4(_logger,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_INFO_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_INFO_L5(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_INFO_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_INFO_L6(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_INFO_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_INFO_L7(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_INFO_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_INFO_L8(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_INFO_9(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_INFO_L9(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_logger),YETI_LOG_LEVEL_INFO,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_FINE(_msg) YETI_LOG_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg))
#define YETI_LOG_FINE_L(_logger,_msg) YETI_LOG_LL((_logger),YETI_LOG_LEVEL_FINE,(_msg))
#define YETI_LOG_FINE_1(_msg,_arg1) YETI_LOG_LL1((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1))
#define YETI_LOG_FINE_L1(_logger,_msg,_arg1) YETI_LOG_LL1((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1))
#define YETI_LOG_FINE_2(_msg,_arg1,_arg2) YETI_LOG_LL2((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2))
#define YETI_LOG_FINE_L2(_logger,_msg,_arg1,_arg2) YETI_LOG_LL2((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2))
#define YETI_LOG_FINE_3(_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_FINE_L3(_logger,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_FINE_4(_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_FINE_L4(_logger,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_FINE_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_FINE_L5(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_FINE_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_FINE_L6(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_FINE_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_FINE_L7(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_FINE_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_FINE_L8(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_FINE_9(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_FINE_L9(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_logger),YETI_LOG_LEVEL_FINE,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_FINER(_msg) YETI_LOG_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg))
#define YETI_LOG_FINER_L(_logger,_msg) YETI_LOG_LL((_logger),YETI_LOG_LEVEL_FINER,(_msg))
#define YETI_LOG_FINER_1(_msg,_arg1) YETI_LOG_LL1((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1))
#define YETI_LOG_FINER_L1(_logger,_msg,_arg1) YETI_LOG_LL1((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1))
#define YETI_LOG_FINER_2(_msg,_arg1,_arg2) YETI_LOG_LL2((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2))
#define YETI_LOG_FINER_L2(_logger,_msg,_arg1,_arg2) YETI_LOG_LL2((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2))
#define YETI_LOG_FINER_3(_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_FINER_L3(_logger,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_FINER_4(_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_FINER_L4(_logger,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_FINER_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_FINER_L5(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_FINER_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_FINER_L6(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_FINER_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_FINER_L7(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_FINER_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_FINER_L8(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_FINER_9(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_FINER_L9(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_logger),YETI_LOG_LEVEL_FINER,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_FINEST(_msg) YETI_LOG_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg))
#define YETI_LOG_FINEST_L(_logger,_msg) YETI_LOG_LL((_logger),YETI_LOG_LEVEL_FINEST,(_msg))
#define YETI_LOG_FINEST_1(_msg,_arg1) YETI_LOG_LL1((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1))
#define YETI_LOG_FINEST_L1(_logger,_msg,_arg1) YETI_LOG_LL1((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1))
#define YETI_LOG_FINEST_2(_msg,_arg1,_arg2) YETI_LOG_LL2((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2))
#define YETI_LOG_FINEST_L2(_logger,_msg,_arg1,_arg2) YETI_LOG_LL2((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2))
#define YETI_LOG_FINEST_3(_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_FINEST_L3(_logger,_msg,_arg1,_arg2,_arg3) YETI_LOG_LL3((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3))
#define YETI_LOG_FINEST_4(_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_FINEST_L4(_logger,_msg,_arg1,_arg2,_arg3,_arg4) YETI_LOG_LL4((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define YETI_LOG_FINEST_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_FINEST_L5(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5) YETI_LOG_LL5((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define YETI_LOG_FINEST_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_FINEST_L6(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) YETI_LOG_LL6((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define YETI_LOG_FINEST_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_FINEST_L7(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) YETI_LOG_LL7((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define YETI_LOG_FINEST_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_FINEST_L8(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) YETI_LOG_LL8((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define YETI_LOG_FINEST_9(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))
#define YETI_LOG_FINEST_L9(_logger,_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9) YETI_LOG_LL9((_logger),YETI_LOG_LEVEL_FINEST,(_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8),(_arg9))

#define YETI_CHECK_FATAL(_result) YETI_CHECK_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_result))
#define YETI_CHECK_FATAL_L(_logger,_result) YETI_CHECK_LL((_logger),YETI_LOG_LEVEL_FATAL,(_result))
#define YETI_CHECK_SEVERE(_result) YETI_CHECK_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_result))
#define YETI_CHECK_SEVERE_L(_logger,_result) YETI_CHECK_LL((_logger),YETI_LOG_LEVEL_SEVERE,(_result))
#define YETI_CHECK_WARNING(_result) YETI_CHECK_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_result))
#define YETI_CHECK_WARNING_L(_logger,_result) YETI_CHECK_LL((_logger),YETI_LOG_LEVEL_WARNING,(_result))
#define YETI_CHECK_INFO(_result) YETI_CHECK_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_result))
#define YETI_CHECK_INFO_L(_logger,_result) YETI_CHECK_LL((_logger),YETI_LOG_LEVEL_INFO,(_result))
#define YETI_CHECK_FINE(_result) YETI_CHECK_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_result))
#define YETI_CHECK_FINE_L(_logger,_result) YETI_CHECK_LL((_logger),YETI_LOG_LEVEL_FINE,(_result))
#define YETI_CHECK_FINER(_result) YETI_CHECK_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_result))
#define YETI_CHECK_FINER_L(_logger,_result) YETI_CHECK_LL((_logger),YETI_LOG_LEVEL_FINER,(_result))
#define YETI_CHECK_FINEST(_result) YETI_CHECK_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_result))
#define YETI_CHECK_FINEST_L(_logger,_result) YETI_CHECK_LL((_logger),YETI_LOG_LEVEL_FINEST,(_result))

#define YETI_CHECK_LABEL_FATAL(_result,_label) YETI_CHECK_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_result),_label)
#define YETI_CHECK_LABEL_FATAL_L(_logger,_result,_label) YETI_CHECK_LABEL_LL((_logger),YETI_LOG_LEVEL_FATAL,(_result),_label)
#define YETI_CHECK_LABEL_SEVERE(_result,_label) YETI_CHECK_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_result),_label)
#define YETI_CHECK_LABEL_SEVERE_L(_logger,_result,_label) YETI_CHECK_LABEL_LL((_logger),YETI_LOG_LEVEL_SEVERE,(_result),_label)
#define YETI_CHECK_LABEL_WARNING(_result,_label) YETI_CHECK_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_result),_label)
#define YETI_CHECK_LABEL_WARNING_L(_logger,_result,_label) YETI_CHECK_LABEL_LL((_logger),YETI_LOG_LEVEL_WARNING,(_result),_label)
#define YETI_CHECK_LABEL_INFO(_result,_label) YETI_CHECK_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_result),_label)
#define YETI_CHECK_LABEL_INFO_L(_logger,_result,_label) YETI_CHECK_LABEL_LL((_logger),YETI_LOG_LEVEL_INFO,(_result),_label)
#define YETI_CHECK_LABEL_FINE(_result,_label) YETI_CHECK_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_result),_label)
#define YETI_CHECK_LABEL_FINE_L(_logger,_result,_label) YETI_CHECK_LABEL_LL((_logger),YETI_LOG_LEVEL_FINE,(_result),_label)
#define YETI_CHECK_LABEL_FINER(_result,_label) YETI_CHECK_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_result),_label)
#define YETI_CHECK_LABEL_FINER_L(_logger,_result,_label) YETI_CHECK_LABEL_LL((_logger),YETI_LOG_LEVEL_FINER,(_result),_label)
#define YETI_CHECK_LABEL_FINEST(_result,_label) YETI_CHECK_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_result),_label)
#define YETI_CHECK_LABEL_FINEST_L(_logger,_result,_label) YETI_CHECK_LABEL_LL((_logger),YETI_LOG_LEVEL_FINEST,(_result),_label)

#define YETI_CHECK_POINTER_FATAL(_p) YETI_CHECK_POINTER_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_p))
#define YETI_CHECK_POINTER_FATAL_L(_logger,_p) YETI_CHECK_POINTER_LL(_logger,YETI_LOG_LEVEL_FATAL,(_p))
#define YETI_CHECK_POINTER_SEVERE(_p) YETI_CHECK_POINTER_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_p))
#define YETI_CHECK_POINTER_SEVERE_L(_logger,_p) YETI_CHECK_POINTER_LL(_logger,YETI_LOG_LEVEL_SEVERE,(_p))
#define YETI_CHECK_POINTER_WARNING(_p) YETI_CHECK_POINTER_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_p))
#define YETI_CHECK_POINTER_WARNING_L(_logger,_p) YETI_CHECK_POINTER_LL(_logger,YETI_LOG_LEVEL_WARNING,(_p))
#define YETI_CHECK_POINTER_INFO(_p) YETI_CHECK_POINTER_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_p))
#define YETI_CHECK_POINTER_INFO_L(_logger,_p) YETI_CHECK_POINTER_LL(_logger,YETI_LOG_LEVEL_INFO,(_p))
#define YETI_CHECK_POINTER_FINE(_p) YETI_CHECK_POINTER_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_p))
#define YETI_CHECK_POINTER_FINE_L(_logger,_p) YETI_CHECK_POINTER_LL(_logger,YETI_LOG_LEVEL_FINE,(_p))
#define YETI_CHECK_POINTER_FINER(_p) YETI_CHECK_POINTER_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_p))
#define YETI_CHECK_POINTER_FINER_L(_logger,_p) YETI_CHECK_POINTER_LL(_logger,YETI_LOG_LEVEL_FINER,(_p))
#define YETI_CHECK_POINTER_FINEST(_p) YETI_CHECK_POINTER_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_p))
#define YETI_CHECK_POINTER_FINEST_L(_logger,_p) YETI_CHECK_POINTER_LL(_logger,YETI_LOG_LEVEL_FINEST,(_p))

#define YETI_CHECK_POINTER_LABEL_FATAL(_p,_label) YETI_CHECK_POINTER_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FATAL,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_FATAL_L(_logger,_p,_label) YETI_CHECK_POINTER_LABEL_LL(_logger,YETI_LOG_LEVEL_FATAL,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_SEVERE(_p,_label) YETI_CHECK_POINTER_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_SEVERE,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_SEVERE_L(_logger,_p,_label) YETI_CHECK_POINTER_LABEL_LL(_logger,YETI_LOG_LEVEL_SEVERE,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_WARNING(_p,_label) YETI_CHECK_POINTER_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_WARNING,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_WARNING_L(_logger,_p,_label) YETI_CHECK_POINTER_LABEL_LL(_logger,YETI_LOG_LEVEL_WARNING,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_INFO(_p,_label) YETI_CHECK_POINTER_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_INFO,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_INFO_L(_logger,_p,_label) YETI_CHECK_POINTER_LABEL_LL(_logger,YETI_LOG_LEVEL_INFO,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_FINE(_p, _label) YETI_CHECK_POINTER_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINE,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_FINE_L(_logger,_p,_label) YETI_CHECK_POINTER_LABEL_LL(_logger,YETI_LOG_LEVEL_FINE,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_FINER(_p,_label) YETI_CHECK_POINTER_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINER,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_FINER_L(_logger,_p,_label) YETI_CHECK_POINTER_LABEL_LL(_logger,YETI_LOG_LEVEL_FINER,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_FINEST(_p,_label) NYETI_CHECK_POINTER_LABEL_LL((_YETI_LocalLogger),YETI_LOG_LEVEL_FINEST,(_p),_label)
#define YETI_CHECK_POINTER_LABEL_FINEST_L(_logger,_p,_label) YETI_CHECK_POINTER_LABEL_LL(_logger,YETI_LOG_LEVEL_FINEST,(_p),_label)

NAMEEND

#endif // _CXL_YETI_LOGGING_H_
