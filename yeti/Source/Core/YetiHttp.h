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
    HttpUrl() {}
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
    HttpEntity(const HttpHeaders & header);
    virtual ~HttpEntity();

    YETI_Result set_input_stream(const InputStreamReference & stream,
        bool update_content_length = false);
    YETI_Result set_input_stream(const void * data, YETI_Size size);
    YETI_Result set_input_stream(const String & string);
    YETI_Result set_input_stream(const char * string);
    YETI_Result get_input_stream(InputStreamReference & stream);
    YETI_Result load(DataBuffer & buffer);
    YETI_Result set_headers(const HttpHeaders & headers);

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
    YETI_Result     set_url(const HttpUrl & url);
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
    struct Config {
        Config() : m_connection_timeout_(YETI_HTTP_CLIENT_DEFAULT_CONNECTION_TIMEOUT),
        m_io_timeout_(YETI_HTTP_CLIENT_DEFAULT_IO_TIMEOUT),
        m_name_resolver_timeout_(YETI_HTTP_CLIENT_DEFAULT_NAME_RESOLVER_TIMEOUT),
        m_max_redirects_(YETI_HTTP_CLIENT_DEFAULT_MAX_REDIRECTS),
        m_user_agent_(YETI_CONFIG_HTTP_DEFAULT_USER_AGENT) {}

        YETI_Timeout m_connection_timeout_;
        YETI_Timeout m_io_timeout_;
        YETI_Timeout m_name_resolver_timeout_;
        YETI_Cardinal m_max_redirects_;
        String m_user_agent_;
    };

    class Connection {
    public:
        virtual ~Connection() {}
        virtual InputStreamReference & get_input_stream() = 0;
        virtual OutputStreamReference & get_output_stream() = 0;
        virtual YETI_Result get_info(SocketInfo & info) = 0;
        virtual bool supports_persistence() { return false; }
        virtual bool is_recycled() { return false; }
        virtual YETI_Result recycle() { delete this; return YETI_SUCCESS; }
        virtual YETI_Result abort() { return YETI_SUCCESS; }
    };

    class ConnectionCanceller {
    public:
        typedef List<Connection *> ConnectionList;

        class Cleaner {
            static Cleaner automatic_cleaner_;
            ~Cleaner() {
                if (instance_) {
                    delete instance_;
                    instance_ = NULL;
                }
            }
        };

        static ConnectionCanceller * get_instance();
        static YETI_Result untrack(Connection * connection);

        ~ConnectionCanceller() {}

        YETI_Result track(HttpClient * client, Connection * connection);
        YETI_Result untrack_connection(Connection * connection);
        YETI_Result abort_connections(HttpClient * client);

    private:
        static ConnectionCanceller * instance_;

        ConnectionCanceller() {}

        Mutex m_lock_;
        Map<HttpClient *, ConnectionList> m_connections_;
        Map<Connection *, HttpClient *> m_clients_;
    };

    class Connector {
    public:
        virtual ~Connector() {}

        virtual YETI_Result connect(const HttpUrl & url,
            HttpClient & client,
            const HttpProxyAddress * proxy,
            bool reuse,
            Connection *& connection) = 0;
        virtual YETI_Result abort() { return YETI_SUCCESS; }

    protected:
        Connector() {}
    };

    static YETI_Result write_quest(OutputStream & output_stream,
        HttpRequest & request,
        bool should_persist,
        bool use_proxy = false);
    static YETI_Result read_response(InputStreamReference & input_stream,
        bool should_persist,
        bool expect_entity,
        HttpResponse *& response,
        Reference<Connection> * cref = NULL);

    HttpClient(Connector * connector = NULL, bool transfer_ownership = true);
    virtual ~HttpClient();

    YETI_Result send_request(HttpRequest & request,
        HttpResponse *& response,
        HttpRequestContext * context = NULL);
    YETI_Result abort();
    const Config & get_config() const { return m_config_; }
    YETI_Result set_config(const Config & config);
    YETI_Result set_proxy(const char * http_proxy_hostname,
        YETI_UInt16 http_proxy_port,
        const char * https_proxy_hostname = NULL,
        YETI_UInt16 https_proxy_port = 0);
    YETI_Result set_proxy_selector(HttpProxySelector * selector);
    YETI_Result set_connector(Connector * connector);
    YETI_Result set_timeouts(YETI_Timeout connection_timeout,
        YETI_Timeout io_timeout,
        YETI_Timeout name_resolver_timeout);
    YETI_Result set_user_agent(const char * user_agent);
    YETI_Result set_options(YETI_Flags options, bool on);

protected:
    YETI_Result _send_request_once(HttpRequest & request,
        HttpResponse *& response,
        HttpRequestContext * context = NULL);

    Config                  m_config_;
    HttpProxySelector *     m_proxy_selector_;
    bool                    m_proxy_selector_is_owned_;
    Connector *             m_connector_;
    bool                    m_connector_is_owned_;
    
    Mutex                   m_abort_lock_;
    bool                    m_aborted_;
};

class HttpConnectionManager : public Thread
{
public:
    class Cleaner {
        static Cleaner automatic_cleaner_;
        ~Cleaner() {
            if (instance_) {
                delete instance_;
                instance_ = NULL;
            }
        }
    };

    static HttpConnectionManager * get_instance();

    class Connection : public HttpClient::Connection
    {
    public:
        Connection(HttpConnectionManager & manager,
            SocketReference & socket,
            InputStreamReference input_stream,
            OutputStreamReference output_steam);
        virtual ~Connection() { HttpClient::ConnectionCanceller::untrack(this); }

        virtual InputStreamReference & get_input_stream() { return m_input_stream_; }
        virtual OutputStreamReference & get_output_stream() { return m_output_stream_; }
        virtual YETI_Result get_info(SocketInfo & info) { return m_socket_->get_info(info); }
        virtual bool supports_persistence() { return true; }
        virtual bool is_recycled() { return m_is_recycled_; }
        virtual YETI_Result recycle();
        virtual YETI_Result abort() { return m_socket_->cancel(); }

        HttpConnectionManager & m_manager_;
        bool                    m_is_recycled_;
        TimeStamp               m_time_stamp_;
        SocketReference         m_socket_;
        InputStreamReference    m_input_stream_;
        OutputStreamReference   m_output_stream_;
    };

    ~HttpConnectionManager();

    Connection * find_connection(SocketAddress & address);
    YETI_Result recycle(Connection * connection);

private:
    static HttpConnectionManager * instance_;
    HttpConnectionManager();

    void run();
    YETI_Result cleanup();

    Mutex               m_lock_;
    YETI_Cardinal       m_max_connections_;
    YETI_Cardinal       m_max_connection_age_;
    List<Connection *>  m_connections_;
    SharedVariable      m_aborted_;
};

class HttpRequestContext
{
public:
    HttpRequestContext() {}
    HttpRequestContext(const SocketAddress * local_address,
        const SocketAddress * remote_address);

    const SocketAddress & get_local_address() const { return m_local_address_; }
    const SocketAddress & get_remote_address() const { return m_remote_address_; }
    void set_local_address(const SocketAddress & address) {
        m_local_address_ = address;
    }
    void set_remote_address(const SocketAddress & address) {
        m_remote_address_ = address;
    }

private:
    SocketAddress m_local_address_;
    SocketAddress m_remote_address_;
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
    HttpStaticRequestHandler(const char * document,
        const char * mime_type = "text/html",
        bool copy = true);
    HttpStaticRequestHandler(const void * data,
        YETI_Size size,
        const char * mime_type = "text/html",
        bool copy = true);

    virtual YETI_Result setup_response(HttpRequest & request,
        const HttpRequestContext & context,
        HttpResponse & response);

private:
    String m_mime_type_;
    DataBuffer m_buffer_;
};

typedef struct HttpFileRequestHandler_DefaultFileTypeMapEntry {
    const char * entension;
    const char * mime_type;
} HttpFileRequestHandler_FileTypeMapEntry;

class HttpFileRequestHandler : public HttpRequestHandler
{
public:
    HttpFileRequestHandler(const char * url_root,
        const char * file_root,
        bool auto_dir = false,
        const char * auto_index = NULL);
    virtual YETI_Result setup_response(
        HttpRequest & request,
        const HttpRequestContext & context,
        HttpResponse & response);

    static const char * get_default_content_type(const char * extension);

    Map<String, String> & get_file_type_map() { return m_file_type_map_; }

    void set_default_mime_type(const char * mime_type) {
        m_default_mime_type_ = mime_type;
    }
    void set_use_default_file_type_map(bool use_default) {
        m_use_default_file_type_map_ = use_default;
    }

    static YETI_Result setup_response_body(HttpResponse & response,
        InputStreamReference & stream,
        const String * range_spec = NULL);

protected:
    const char * get_content_type(const String & filename);

private:
    String m_url_root_;
    String m_file_root_;
    Map<String, String> m_file_type_map_;
    String m_default_mime_type_;
    bool m_use_default_file_type_map_;
    bool m_auto_dir_;
    String m_auto_index_;
};

class HttpServer
{
public:
    struct Config {
        YETI_Timeout m_conneciton_timeout_;
        YETI_Timeout m_io_timeout_;
        IpAddress m_listen_address_;
        YETI_UInt16 m_listen_port_;
        bool m_reuse_address_;
    };

    HttpServer(YETI_UInt16 listen_port = YETI_HTTP_DEFAULT_PORT,
        bool reuse_address = true);
    HttpServer(IpAddress listen_address,
        YETI_UInt16 listen_port = YETI_HTTP_DEFAULT_PORT,
        bool reuse_address = true);

    virtual ~HttpServer();

    YETI_Result set_config(const Config & config);
    const Config & get_config() const { return m_config_; }
    YETI_Result set_listen_port(YETI_UInt16 port, bool reuse_address = true);
    YETI_Result set_timeouts(YETI_Timeout  connection_timeout, YETI_Timeout io_timeout);
    YETI_Result set_server_header(const char * server_header);
    YETI_Result abort();
    YETI_Result wait_for_new_client(InputStreamReference & input,
        OutputStreamReference & output,
        HttpRequestContext * context,
        YETI_Flags socket_flags = 0);
    YETI_Result loop(bool cancellable_sockets = true);
    YETI_UInt16 get_port() { return m_bound_port_; }
    void terminate();

    virtual YETI_Result add_request_handler(HttpRequestHandler * handler,
        const char * path,
        bool include_children = false,
        bool transfer_ownership = false);
    virtual HttpRequestHandler * find_request_handler(HttpRequest & request);
    virtual List<HttpRequestHandler *> find_request_handlers(HttpRequest & request);

    virtual YETI_Result respond_to_client(InputStreamReference & input,
        OutputStreamReference & output,
        const HttpRequestContext & context);

protected:
    struct HandlerConfig {
        HandlerConfig(HttpRequestHandler * handler,
            const char * path,
            bool include_children,
            bool transfer_ownership = false);
        ~HandlerConfig();

        HttpRequestHandler * m_handler_;
        String m_path_;
        bool m_include_children_;
        bool m_handler_is_owned_;
    };

    YETI_Result bind();
    TcpServerSocket m_socket_;
    YETI_UInt16 m_bound_port_;
    Config m_config_;
    List<HandlerConfig *> m_request_handlers_;
    String m_server_header_;
    bool m_run_;
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
