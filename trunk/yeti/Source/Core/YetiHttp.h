#ifndef _CXL_YETI_HTTP_H_
#define _CXL_YETI_HTTP_H_

#include "YetiUri.h"
#include "YetiTypes.h"
#include "YetiList.h"
#include "YetiBufferedStream.h"
#include "YetiSocket.h"
#include "YetiMap.h"
#include "YetiDynamicCast.h"
#include "YetiVersion.h"
#include "YetiTime.h"
#include "YetiThreads.h"

#define YETI_HTTP_PROTOCOL_1_0   "HTTP/1.0"
#define YETI_HTTP_PROTOCOL_1_1   "HTTP/1.1"
#define YETI_HTTP_METHOD_GET     "GET"
#define YETI_HTTP_METHOD_HEAD    "HEAD"
#define YETI_HTTP_METHOD_POST    "POST"
#define YETI_HTTP_METHOD_PUT     "PUT"
#define YETI_HTTP_METHOD_OPTIONS "OPTIONS"
#define YETI_HTTP_METHOD_DELETE  "DELETE"
#define YETI_HTTP_METHOD_TRACE   "TRACE"

#define YETI_HTTP_HEADER_HOST                "Host"
#define YETI_HTTP_HEADER_CONNECTION          "Connection"
#define YETI_HTTP_HEADER_USER_AGENT          "User-Agent"
#define YETI_HTTP_HEADER_SERVER              "Server"
#define YETI_HTTP_HEADER_CONTENT_LENGTH      "Content-Length"
#define YETI_HTTP_HEADER_CONTENT_TYPE        "Content-Type"
#define YETI_HTTP_HEADER_CONTENT_ENCODING    "Content-Encoding"
#define YETI_HTTP_HEADER_TRANSFER_ENCODING   "Transfer-Encoding"
#define YETI_HTTP_HEADER_LOCATION            "Location"
#define YETI_HTTP_HEADER_RANGE               "Range"
#define YETI_HTTP_HEADER_CONTENT_RANGE       "Content-Range"
#define YETI_HTTP_HEADER_COOKIE              "Cookie"
#define YETI_HTTP_HEADER_ACCEPT_RANGES       "Accept-Ranges"
#define YETI_HTTP_HEADER_CONTENT_RANGE       "Content-Range"
#define YETI_HTTP_HEADER_AUTHORIZATION       "Authorization"

#define YETI_HTTP_TRANSFER_ENCODING_CHUNKED  "chunked"

#define YETI_HTTP_LINE_TERMINATOR "\r\n"

#if !defined(YETI_CONFIG_HTTP_DEFAULT_USER_AGENT)
#define YETI_CONFIG_HTTP_DEFAULT_USER_AGENT "Yeti/"YETI_VERSION_STRING
#endif

NAMEBEG
const unsigned int YETI_HTTP_DEFAULT_PORT  = 80;
const unsigned int YETI_HTTPS_DEFAULT_PORT = 443;
const unsigned int YETI_HTTP_INVALID_PORT  = 0;

const YETI_Timeout  YETI_HTTP_CLIENT_DEFAULT_CONNECTION_TIMEOUT    = 30000;
const YETI_Timeout  YETI_HTTP_CLIENT_DEFAULT_IO_TIMEOUT            = 30000;
const YETI_Timeout  YETI_HTTP_CLIENT_DEFAULT_NAME_RESOLVER_TIMEOUT = 60000;
const unsigned int  YETI_HTTP_CLIENT_DEFAULT_MAX_REDIRECTS         = 20;

const YETI_Timeout YETI_HTTP_SERVER_DEFAULT_CONNECTION_TIMEOUT    = YETI_TIMEOUT_INFINITE;
const YETI_Timeout YETI_HTTP_SERVER_DEFAULT_IO_TIMEOUT            = 60000;

const unsigned int YETI_HTTP_CONNECTION_MANAGER_MAX_CONNECTION_POOL_SIZE = 5;
const unsigned int YETI_HTTP_CONNECTION_MANAGER_MAX_CONNECTION_AGE       = 50; // seconds
const unsigned int YETI_HTTP_MAX_RECONNECTS                              = 10;
const unsigned int YETI_HTTP_MAX_100_RESPONSES                           = 10;

const int YETI_HTTP_PROTOCOL_MAX_LINE_LENGTH  = 8192;
const int YETI_HTTP_PROTOCOL_MAX_HEADER_COUNT = 100;

const int YETI_ERROR_HTTP_INVALID_RESPONSE_LINE = YETI_ERROR_BASE_HTTP - 0;
const int YETI_ERROR_HTTP_INVALID_REQUEST_LINE  = YETI_ERROR_BASE_HTTP - 1;
const int YETI_ERROR_HTTP_NO_PROXY              = YETI_ERROR_BASE_HTTP - 2;
const int YETI_ERROR_HTTP_INVALID_REQUEST       = YETI_ERROR_BASE_HTTP - 3;
const int YETI_ERROR_HTTP_METHOD_NOT_SUPPORTED  = YETI_ERROR_BASE_HTTP - 4;
const int YETI_ERROR_HTTP_TOO_MANY_REDIRECTS    = YETI_ERROR_BASE_HTTP - 5;
const int YETI_ERROR_HTTP_TOO_MANY_RECONNECTS   = YETI_ERROR_BASE_HTTP - 6;
const int YETI_ERROR_HTTP_CANNOT_RESEND_BODY    = YETI_ERROR_BASE_HTTP - 7;

typedef unsigned int HttpStatusCode;
typedef UrlQuery HttpUrlQuery; 

class HttpUrl : public Url
{
public:
    HttpUrl();
    HttpUrl(const char * host,
        YETI_UInt16 port,
        const char * path,
        const char * query = NULL,
        const char * fragment = NULL);
    HttpUrl(const char * url, bool ignore_scheme = false);

    virtual String to_string(bool with_fragment = true) const;
};

class HttpProtocol
{
public:
    const char * get_status_code_string(HttpStatusCode status_code);
};

class HttpHeader
{
public:
private:
};

class HttpHeaders
{
public:
private:
};

NAMEEND

#endif // _CXL_YETI_HTTP_H_
