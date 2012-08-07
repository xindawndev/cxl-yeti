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
    HttpHeader(const char * name, const char * value);
    ~HttpHeader();

    YETI_Result emit(OutputStream & stream) const;
    const String & get_name() const { return m_name_; }
    const String & get_value() const { return m_value_; }
    YETI_Result set_name(const char * name);
    YETI_Result set_value(const char * value);

private:
    String m_name_;
    String m_value_;
};

class HttpHeaders
{
public:
    HttpHeaders();
    ~HttpHeaders();

    YETI_Result parse(BufferedInputStream & stream);
    YETI_Result emit(OutputStream & stream) const;
    const List<HttpHeader *> & get_headers() const { return m_headers_; }
    HttpHeader * get_header(const char * name) const;
    const String * get_header_value(const char * name) const;
    YETI_Result set_header(const char * name, const char * value, bool replace = true);
    YETI_Result add_header(const char * name, const char * value);
    YETI_Result remove_header(const char * name);

private:
    List<HttpHeader *> m_headers_;
};

class HttpEntity
{
public:
    HttpEntity();
    HttpEntity(const HttpHeaders & headers);
    virtual ~HttpEntity();

    YETI_Result set_input_stream(const InputStreamReference & stream,
        bool update_content_length = false);
    YETI_Result set_input_stream(const void * data, YETI_Size size);
    YETI_Result set_input_stream(const String & string);
    YETI_Result set_input_stream(const char * string);
    YETI_Result set_input_stream(InputStreamReference & stream);

    YETI_Result         set_content_length(YETI_LargeSize length);
    YETI_Result         set_content_type(const char * type);
    YETI_Result         set_content_encoding(const char * encoding);
    YETI_Result         set_transfer_encoding(const char * encoding);
    YETI_LargeSize      get_content_length() { return m_content_length_; }
    const String &      get_content_type() { return m_content_type_; }
    const String &      get_content_encoding() { return m_content_encoding_; }
    const String &      get_transfer_encoding() { return m_transfer_encoding_; }
    bool                content_length_is_known() { return m_content_length_is_known_; }

private:
    InputStreamReference m_input_stream_;
    YETI_LargeSize       m_content_length_;
    String               m_content_type_;
    String               m_content_encoding_;
    String               m_transfer_encoding_;
    bool                 m_content_length_is_known_;
};

class HttpMessage
{
public:
    virtual ~HttpMessage();

    const String & get_protocol() const {
        return m_protocol_;
    }

    YETI_Result set_protocol(const char * protocol) {
        m_protocol_ = protocol;
        return YETI_SUCCESS;
    }
    HttpHeaders & get_headers() {
        return m_headers_;
    }
    const HttpHeaders & get_headers() const {
        return m_headers_;
    }
    YETI_Result set_entity(HttpEntity * entity);
    HttpEntity * get_entity() {
        return m_entity_;
    }
    virtual YETI_Result parse_headers(BufferedInputStream & stream);

protected:
    HttpMessage(const char * protocol);

    String          m_protocol_;
    HttpHeaders     m_headers_;
    HttpEntity *    m_entity_;
};

class HttpRequest : public HttpMessage
{
public:
    static YETI_Result parse(BufferedInputStream & stream,
        const SocketAddress * endpoint,
        HttpRequest *& request);

    HttpRequest(const HttpUrl & url,
        const char * method,
        const char * protocol = YETI_HTTP_PROTOCOL_1_0);
    HttpRequest(const char * url,
        const char * method,
        const char * protocol = YETI_HTTP_PROTOCOL_1_0);
    virtual ~HttpRequest();

    const HttpUrl & get_url() const { return m_url_; }
    HttpUrl &       get_url() { return m_url_; }
    YETI_Result     set_url(const char * url);
    const String &  get_method() const { return m_method_; }
    virtual YETI_Result emit(OutputStream & stream, bool use_proxy = false) const;

protected:
    HttpUrl m_url_;
    String m_method_;
};

class HttpResponse : public HttpMessage
{
public:
    static YETI_Result parse(BufferedInputStream & stream,
        HttpResponse *& response);

    HttpResponse(HttpStatusCode status_code,
        const char * reason_phrase,
        const char * protocol = YETI_HTTP_PROTOCOL_1_0);
    virtual ~HttpResponse();

    YETI_Result set_status(HttpStatusCode status_code,
        const char * reason_phrase,
        const char * protocol = NULL);
    YETI_Result set_protocol(const char * protocol);
    HttpStatusCode get_status_code() const { return m_status_code_; }
    const String & get_reason_phrase() const { return m_reason_phrase_; }
    virtual YETI_Result emit(OutputStream & stream) const;
protected:
    HttpStatusCode m_status_code_;
    String         m_reason_phrase_;
};

class HttpProxyAddress
{
public:
    HttpProxyAddress()
        : m_port_(YETI_HTTP_INVALID_PORT) {}
    HttpProxyAddress(const char * hostname, YETI_UInt16 port)
        : m_hostname_(hostname)
        , m_port_(port) {}

    const String & get_hostname() const { return m_hostname_; }
    void           set_hostname(const char * hostname) { m_hostname_ = hostname; }
    YETI_UInt16    get_port() const { return m_port_; }
    void           set_port(YETI_UInt16 port) { m_port_ = port; }

private:
    String      m_hostname_;
    YETI_UInt16 m_port_;
};

class HttpProxySelector
{
public:
    static HttpProxySelector * get_default();
    static HttpProxySelector * get_system_selector();

    virtual ~HttpProxySelector() {}
    virtual YETI_Result get_proxy_for_url(const HttpUrl & url, HttpProxyAddress & proxy) = 0;
private:
    static HttpProxySelector * m_system_default_;
};

class HttpRequestContext;

class HttpClient
{
public:
private:
};

class HttpConnectionManager : public Thread
{
public:
private:
};

class HttpRequestContext
{
public:
private:
};

class HttpRequestHandler
{
public:
    YETI_IMPLEMENT_DYNAMIC_CAST(HttpRequestHandler)

    virtual ~HttpRequestHandler() {}

    virtual YETI_Result setup_response(HttpRequest & request,
        const HttpRequestContext & context,
        HttpResponse & response) = 0;
    virtual YETI_Result send_response_body(const HttpRequestContext & context,
        HttpResponse & response,
        OutputStream & output);
};

class HttpStaticRequestHandler : public HttpRequestHandler
{
public:
private:
};

typedef struct HttpFileRequestHandler_DefaultFileTypeMapEntry {
    const char * entension;
    const char * mime_type;
} HttpFileRequestHandler_FileTypeMapEntry;

class HttpFileRequestHandler : public HttpRequestHandler
{
public:
protected:
private:
};

class HttpServer
{
public:
protected:
};

class HttpResponder
{
public:
    struct Config {
        YETI_Timeout m_io_timeout_;
    };
    HttpResponder(InputStreamReference & input,
        OutputStreamReference & output);
    virtual ~HttpResponder();

    YETI_Result set_config(const Config & config);
    YETI_Result set_timeout(YETI_Timeout io_timeout);
    YETI_Result parse_request(HttpRequest *& request,
        const SocketAddress * local_address = NULL);
    YETI_Result send_response_headers(HttpResponse & reponse);

protected:
    Config                       m_config_;
    BufferedInputStreamReference m_input_;
    OutputStreamReference        m_output_;
};

class HttpChunkedInputStream : public InputStream
{
public:
    HttpChunkedInputStream(BufferedInputStreamReference & stream);
    virtual ~HttpChunkedInputStream();

    YETI_Result read(void * buffer, YETI_Size bytes_to_read, YETI_Size * bytes_read  = NULL );
    YETI_Result seek(YETI_Position offset);
    YETI_Result tell(YETI_Position & offset);
    YETI_Result get_size(YETI_LargeSize & size);
    YETI_Result get_available(YETI_LargeSize & available);

protected:
    BufferedInputStreamReference m_source_;
    YETI_UInt32                  m_current_chunck_size_;
    bool                         m_eos_;
};

class HttpChunkedOutputStream : public OutputStream
{
public:
    HttpChunkedOutputStream(OutputStream & stream);
    virtual ~HttpChunkedOutputStream();

    YETI_Result write(const void * buffer,
        YETI_Size bytes_to_write,
        YETI_Size * bytes_written = NULL);
    YETI_Result seek(YETI_Position /*offset*/)  { return YETI_ERROR_NOT_SUPPORTED; }
    YETI_Result tell(YETI_Position & offset)    { return m_stream_.tell(offset); }
    YETI_Result flush()                         { return m_stream_.flush(); }

protected:
    OutputStream & m_stream_;
};

NAMEEND

#endif // _CXL_YETI_HTTP_H_
