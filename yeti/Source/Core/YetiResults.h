#ifndef _CXL_YETI_RESULTS_H_
#define _CXL_YETI_RESULTS_H_

#if defined(YETI_DEBUG)

#include "YetiDebug.h"

#define YETI_CHECK(_x)                          \
    do {                                        \
        YETI_Result _result = (_x);             \
        if (_result != YETI_SUCCESS) {          \
            yeti_debug("%s(%d): @@@ YETI_CHECK failed, result=%d (%s)\n", __FILE__, __LINE__, _result, YETI_ResultText(_result)); \
            return _result;                     \
        }                                       \
    } while(0)

#define YETI_CHECK_POINTER(_p)                  \
    do {                                        \
        if ((_p) == NULL) {                     \
            yeti_debug("%s(%d): @@@ NULL pointer parameter\n", __FILE__, __LINE__); \
            return YETI_ERROR_INVALID_PARAMETERS; \
        }                                       \
    } while(0)

#define YETI_CHECK_LABEL(_x, label)             \
    do {                                        \
        YETI_Result _result = (_x);             \
        if (_result != YETI_SUCCESS) {          \
            yeti_debug("%s(%d): @@@ YETI_CHECK failed, result=%d (%s)\n", __FILE__, __LINE__, _result, YETI_ResultText(_result)); \
            goto label;                         \
        }                                       \
    } while(0)

#define YETI_CHECK_POINTER_LABEL(_p, label)     \
    do {                                        \
        if (_p == NULL) {                       \
            yeti_debug("%s(%d): @@@ NULL pointer parameter\n", __FILE__, __LINE__); \
            goto label;                         \
        }                                       \
    } while(0)

#else

#define YETI_CHECK(_x)                  \
    do{                                 \
        YETI_Result _result = (_x);     \
        if (_result != YETI_SUCCESS) {  \
            return _result;             \
        }                               \
    } while(0)

#define YETI_CHECK_POINTER(_p)                                  \
    do {                                                        \
        if (NULL == (_p)) return YETI_ERROR_INVALID_PARAMETERS; \
    } while (0)

#define YETI_CHECK_LABEL(_x, label)     \
    do {                                \
        YETI_Result _result = (_x);     \
        if (YETI_SUCCESS != _result) {  \
            goto label;                 \
        }                               \
    } while(0)

#define YETI_CHECK_POINTER_LABEL(_p, label)     \
    do {                                        \
        if (NULL == (_p))                       \
            goto label;                         \
    } while(0)

#endif

#define YETI_FAILED(result)         ((result) != YETI_SUCCESS)
#define YETI_SUCCEEDED(result)      ((result) == YETI_SUCCESS)

#define YETI_SUCCESS                0
#define YETI_FAILURE                (-1)

#if !defined(YETI_ERROR_BASE)
#   define YETI_ERROR_BASE          -20000
#endif

// error bases
#define YETI_ERROR_BASE_GENERAL        (YETI_ERROR_BASE - 0)               // general
#define YETI_ERROR_BASE_LIST           (YETI_ERROR_BASE - 100)             // list
#define YETI_ERROR_BASE_FILE           (YETI_ERROR_BASE - 200)             // file
#define YETI_ERROR_BASE_IO             (YETI_ERROR_BASE - 300)             // io
#define YETI_ERROR_BASE_SOCKET         (YETI_ERROR_BASE - 400)             // socket
#define YETI_ERROR_BASE_INTERFACES     (YETI_ERROR_BASE - 500)             // interface
#define YETI_ERROR_BASE_XML            (YETI_ERROR_BASE - 600)             // xml
#define YETI_ERROR_BASE_UNIX           (YETI_ERROR_BASE - 700)             // unix
#define YETI_ERROR_BASE_HTTP           (YETI_ERROR_BASE - 800)             // http
#define YETI_ERROR_BASE_THREADS        (YETI_ERROR_BASE - 900)             // threads
#define YETI_ERROR_BASE_SERIAL_PORT    (YETI_ERROR_BASE - 1000)            // serial port
#define YETI_ERROR_BASE_TLS            (YETI_ERROR_BASE - 1100)            // tls

// general errors
#define YETI_ERROR_INVALID_PARAMETERS  (YETI_ERROR_BASE_GENERAL - 0)     // invalid parameters
#define YETI_ERROR_PERMISSION_DENIED   (YETI_ERROR_BASE_GENERAL - 1)     // permission denied
#define YETI_ERROR_OUT_OF_MEMORY       (YETI_ERROR_BASE_GENERAL - 2)     // out of memory
#define YETI_ERROR_NO_SUCH_NAME        (YETI_ERROR_BASE_GENERAL - 3)     // no such name
#define YETI_ERROR_NO_SUCH_PROPERTY    (YETI_ERROR_BASE_GENERAL - 4)     // no such property
#define YETI_ERROR_NO_SUCH_ITEM        (YETI_ERROR_BASE_GENERAL - 5)     // no such item
#define YETI_ERROR_NO_SUCH_CLASS       (YETI_ERROR_BASE_GENERAL - 6)     // no such class
#define YETI_ERROR_OVERFLOW            (YETI_ERROR_BASE_GENERAL - 7)     // overflow
#define YETI_ERROR_INTERNAL            (YETI_ERROR_BASE_GENERAL - 8)     // internal
#define YETI_ERROR_INVALID_STATE       (YETI_ERROR_BASE_GENERAL - 9)     // invalid state
#define YETI_ERROR_INVALID_FORMAT      (YETI_ERROR_BASE_GENERAL - 10)    // invalid format
#define YETI_ERROR_INVALID_SYNTAX      (YETI_ERROR_BASE_GENERAL - 11)    // invalid syntax
#define YETI_ERROR_NOT_IMPLEMENTED     (YETI_ERROR_BASE_GENERAL - 12)    // not implemented
#define YETI_ERROR_NOT_SUPPORTED       (YETI_ERROR_BASE_GENERAL - 13)    // not supported
#define YETI_ERROR_TIMEOUT             (YETI_ERROR_BASE_GENERAL - 14)    // timeout
#define YETI_ERROR_WOULD_BLOCK         (YETI_ERROR_BASE_GENERAL - 15)    // would block
#define YETI_ERROR_TERMINATED          (YETI_ERROR_BASE_GENERAL - 16)    // terminated
#define YETI_ERROR_OUT_OF_RANGE        (YETI_ERROR_BASE_GENERAL - 17)    // out of range
#define YETI_ERROR_OUT_OF_RESOURCES    (YETI_ERROR_BASE_GENERAL - 18)    // resources
#define YETI_ERROR_NOT_ENOUGH_SPACE    (YETI_ERROR_BASE_GENERAL - 19)    // no enough space
#define YETI_ERROR_INTERRUPTED         (YETI_ERROR_BASE_GENERAL - 20)    // interrupted
#define YETI_ERROR_CANCELLED           (YETI_ERROR_BASE_GENERAL - 21)    // cancelled

#define YETI_ERROR_BASE_ERRNO          (YETI_ERROR_BASE - 2000)          // base errno
#define YETI_ERROR_ERRNO(e)            (YETI_ERROR_BASE_ERRNO - (e))     // errno

const char * error_message(int result);

#endif // _CXL_YETI_RESULTS_H_
