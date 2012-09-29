#include "YetiHttp.h"
#include "YetiSocket.h"
#include "YetiBufferedStream.h"
#include "YetiDebug.h"
#include "YetiVersion.h"
#include "YetiUtil.h"
#include "YetiFile.h"
#include "YetiSystem.h"
#include "YetiLogging.h"
//#include "YetiTls.h"
#include "YetiStreams.h"

YETI_SET_LOCAL_LOGGER("yeti.http")


NAMEBEG
const char* const YETI_HTTP_DEFAULT_403_HTML = "<html><head><title>403 Forbidden</title></head><body><h1>Forbidden</h1><p>Access to this URL is forbidden.</p></html>";
const char* const YETI_HTTP_DEFAULT_404_HTML = "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></html>";
const char* const YETI_HTTP_DEFAULT_500_HTML = "<html><head><title>500 Internal Error</title></head><body><h1>Internal Error</h1><p>The server encountered an unexpected condition which prevented it from fulfilling the request.</p></html>";

HttpUrl::HttpUrl(const char * host,
                 YETI_UInt16 port,
                 const char * path,
                 const char * query /*= NULL*/,
                 const char * fragment/* = NULL*/)
                 : Url("http", host, port, path, query, fragment)
{
}

HttpUrl::HttpUrl(const char * url, bool ignore_scheme /*= false*/)
: Url(url)
{
    if (!ignore_scheme) {
        if (get_schemeid() != Uri::SCHEME_ID_HTTP &&
            get_schemeid() != Uri::SCHEME_ID_HTTPS) {
                reset();
        }
    }
}

String HttpUrl::to_string(bool with_fragment /*= true*/) const
{
    YETI_UInt16 default_port;
    switch (m_schemeid_)
    {
    case SCHEME_ID_HTTP: default_port = YETI_HTTP_DEFAULT_PORT; break;
    case SCHEME_ID_HTTPS: default_port = YETI_HTTPS_DEFAULT_PORT; break;
    default: default_port = 0;
    }
    return Url::to_string_with_default_port(default_port, with_fragment);
}

const char * HttpProtocol::get_status_code_string(HttpStatusCode status_code)
{
    return NULL;
}

HttpHeader::HttpHeader(const char * name, const char * value)
: m_name_(name)
, m_value_(value)
{
}

HttpHeader::~HttpHeader()
{
}

YETI_Result HttpHeader::emit(OutputStream & stream) const
{
    stream.write_string(m_name_);
    stream.write_fully(": ", 2);
    stream.write_string(m_value_);
    stream.write_fully(YETI_HTTP_LINE_TERMINATOR, 2);
    YETI_LOG_FINEST_2("header %s : %s", m_name_.get_chars(), m_value_.get_chars());

    return YETI_SUCCESS;
}

YETI_Result HttpHeader::set_name(const char * name)
{
    m_name_ = name;
    return YETI_SUCCESS;
}

YETI_Result HttpHeader::set_value(const char * value)
{
    m_value_ = value;
    return YETI_SUCCESS;
}

HttpHeaders::HttpHeaders()
{
}

HttpHeaders::~HttpHeaders()
{
    m_headers_.apply(ObjectDeleter<HttpHeader>());
}

YETI_Result HttpHeaders::parse(BufferedInputStream & stream)
{
    String header_name;
    String header_value;
    bool   header_pending = false;
    String line;

    while (YETI_SUCCEEDED(stream.read_line(line, YETI_HTTP_PROTOCOL_MAX_LINE_LENGTH))) {
        if (line.get_length() == 0) {
            // end of headers
            break;
        }
        if (header_pending && (line[0] == ' ' || line[0] == '\t')) {
            header_value.append(line.get_chars() + 1, line.get_length() - 1);
        } else {
            if (header_pending) {
                header_value.trim();
                add_header(header_name, header_value);
                header_pending = false;
                YETI_LOG_FINEST_2("header - %s: %s", header_name.get_chars(), header_value.get_chars());
            }

            int colon_index = line.find(':');
            if (colon_index < 1) {
                // invalid syntax, ignore
                continue;
            }
            header_name = line.left(colon_index);
            const char * value = line.get_chars() + colon_index + 1;
            while (*value == ' ' || *value == '\t') {
                value++;
            }
            header_value =  value;
            header_pending = true;
        }
    }

    if (header_pending) {
        header_value.trim();
        add_header(header_name, header_value);
        YETI_LOG_FINEST_2("header - %s: %s", header_name.get_chars(), header_value.get_chars());
    }

    return YETI_SUCCESS;
}

YETI_Result HttpHeaders::emit(OutputStream & stream) const
{
    List<HttpHeader *>::iterator header = m_headers_.get_first_item();
    while (header) {
        YETI_CHECK_WARNING((*header)->emit(stream));
        ++header;
    }
    return YETI_SUCCESS;
}

HttpHeader * HttpHeaders::get_header(const char * name) const
{
    if (name == NULL) return NULL;
    List<HttpHeader *>::iterator header = m_headers_.get_first_item();
    while (header) {
        if ((*header)->get_name().compare(name, true) == 0) {
            return *header;
        }
        ++header;
    }
    return NULL;
}

const String * HttpHeaders::get_header_value(const char * name) const
{
    HttpHeader * header = get_header(name);
    if (header == NULL) {
        return NULL;
    } else {
        return &header->get_value();
    }
}

YETI_Result HttpHeaders::set_header(const char * name, const char * value, bool replace/* = true*/)
{
    HttpHeader * header = get_header(name);
    if (header == NULL) {
        return add_header(name, value);
    } else if (replace) {
        return header->set_value(value);
    } else  {
        return YETI_SUCCESS;
    }
}

YETI_Result HttpHeaders::add_header(const char * name, const char * value)
{
    return m_headers_.add(new HttpHeader(name, value));
}

YETI_Result HttpHeaders::remove_header(const char * name)
{
    bool found = false;
    HttpHeader * header = NULL;
    while (header = get_header(name)) {
        m_headers_.remove(header);
        delete header;
        found = true;
    }
    return found ? YETI_SUCCESS : YETI_ERROR_NO_SUCH_ITEM;
}

class HttpEntityBodyInputStream : public InputStream
{
public:
    HttpEntityBodyInputStream(BufferedInputStreamReference& source,
        YETI_LargeSize                    size,
        bool                              size_is_known,
        bool                              chunked,
        HttpClient::Connection*           connection,
        bool                              should_persist)
        : m_size_(size)
        , m_size_is_known_(size_is_known)
        , m_is_chunked_(chunked)
        , m_connection_(connection)
        , m_should_persist_(should_persist)
        , m_position_(0) {
            if (size_is_known && size == 0) {
                on_fully_read();
            } else {
                if (chunked) {
                    m_source_ = InputStreamReference(new HttpChunkedInputStream(source));
                } else {
                    m_source_ = source;
                }
            }
    }

    virtual ~HttpEntityBodyInputStream() {
        delete m_connection_;
        m_connection_ = NULL;
    }

    bool size_is_known() { return m_size_is_known_; }

    virtual YETI_Result read(void * buffer,
        YETI_Size bytes_to_read,
        YETI_Size * bytes_read = NULL);
    virtual YETI_Result seek(YETI_Position offset) {
        return YETI_ERROR_NOT_SUPPORTED;
    }
    virtual YETI_Result tell(YETI_Position & offset) {
        offset = m_position_;
        return YETI_SUCCESS;
    }
    virtual YETI_Result get_size(YETI_LargeSize & size) {
        size = m_size_;
        return YETI_SUCCESS;
    }
    virtual YETI_Result get_available(YETI_LargeSize & available);
private:
    virtual void on_fully_read();

    YETI_LargeSize           m_size_;
    bool                     m_size_is_known_;
    bool                     m_is_chunked_;
    HttpClient::Connection * m_connection_;
    bool                     m_should_persist_;
    YETI_Position            m_position_;
    InputStreamReference     m_source_;
};

void HttpEntityBodyInputStream::on_fully_read()
{
    m_source_ = NULL;
    if (m_connection_ && m_should_persist_) {
        m_connection_->recycle();
        m_connection_ = NULL;
    }
}

YETI_Result HttpEntityBodyInputStream::read(void * buffer, YETI_Size bytes_to_read, YETI_Size * bytes_read /* = NULL */)
{
    if (bytes_read) *bytes_read = 0;
    if (m_source_.is_null()) return YETI_ERROR_EOS;
    if (!m_is_chunked_ && m_size_is_known_) {
        YETI_LargeSize max_can_read = m_size_ - m_position_;
        if (max_can_read == 0) return YETI_ERROR_EOS;
        if (bytes_to_read > max_can_read) bytes_to_read = (YETI_Size)max_can_read;
    }

    YETI_Size source_bytes_read = 0;
    YETI_Result result = m_source_->read(buffer, bytes_to_read, &source_bytes_read);
    if (YETI_SUCCEEDED(result)) {
        m_position_ += source_bytes_read;
        if (bytes_read) *bytes_read = source_bytes_read;
    }

    if (result == YETI_ERROR_EOS || (m_size_is_known_ && (m_position_ == m_size_))) {
        on_fully_read();
    }

    return result;
}

YETI_Result HttpEntityBodyInputStream::get_available(YETI_LargeSize & available)
{
    if (m_source_.is_null()) {
        available = 0;
        return YETI_SUCCESS;
    }

    YETI_Result result = m_source_->get_available(available);
    if (YETI_FAILED(result)) {
        available = 0;
        return result;
    }
    if (available > m_size_ - m_position_) {
        available = m_size_ - m_position_;
    }

    return YETI_SUCCESS;
}

HttpEntity::HttpEntity()
: m_content_length_(0)
, m_content_length_is_known_(false)
{
}

HttpEntity::HttpEntity(const HttpHeaders & header)
: m_content_length_(0)
, m_content_length_is_known_(false)
{
    set_headers(header);
}

HttpEntity::~HttpEntity()
{

}

YETI_Result HttpEntity::set_input_stream(const InputStreamReference & stream,
                                         bool update_content_length/* = false*/)
{
    m_input_stream_ = stream;
    if (update_content_length && !stream.is_null()) {
        YETI_LargeSize length;
        if (YETI_SUCCEEDED(stream->get_size(length))) {
            return set_content_length(length);
        }
    }
    return YETI_SUCCESS;
}

YETI_Result HttpEntity::set_input_stream(const void * data, YETI_Size size)
{
    MemoryStream * memory_stream = new MemoryStream(data, size);
    InputStreamReference body(memory_stream);
    return set_input_stream(body, true);
}

YETI_Result HttpEntity::set_input_stream(const String & string)
{
    MemoryStream * memory_stream = new MemoryStream((const void *)string.get_chars(), string.get_length());
    InputStreamReference body(memory_stream);
    return set_input_stream(body, true);
}

YETI_Result HttpEntity::set_input_stream(const char * string)
{
    if (string == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    MemoryStream * memory_stream = new MemoryStream((const void *)string, StringLength(string));
    InputStreamReference body(memory_stream);
    return set_input_stream(string, true);
}

YETI_Result HttpEntity::get_input_stream(InputStreamReference & stream)
{
    stream = NULL;
    if (m_input_stream_.is_null()) return YETI_FAILURE;
    stream = m_input_stream_;
    return YETI_SUCCESS;
}

YETI_Result HttpEntity::load(DataBuffer & buffer)
{
    if (m_input_stream_.is_null()) return YETI_ERROR_INVALID_STATE;
    if (m_content_length_ != (YETI_Size)m_content_length_) return YETI_ERROR_OUT_OF_RANGE;
    return m_input_stream_->load(buffer, (YETI_Size)m_content_length_);
}

YETI_Result HttpEntity::set_headers(const HttpHeaders & headers)
{
    HttpHeader * header;
    header = headers.get_header(YETI_HTTP_HEADER_CONTENT_LENGTH);
    if (header != NULL) {
        m_content_length_is_known_ = true;
        YETI_LargeSize length;
        if (YETI_SUCCEEDED(header->get_value().to_integer64(length))) {
            m_content_length_ = length;
        } else {
            m_content_length_ = 0;
        }
    }

    header = headers.get_header(YETI_HTTP_HEADER_CONTENT_TYPE);
    if (header != NULL) {
        m_content_type_ = header->get_value();
    }

    header = headers.get_header(YETI_HTTP_HEADER_CONTENT_ENCODING);
    if (header != NULL) {
        m_content_encoding_ = header->get_value();
    }

    header = headers.get_header(YETI_HTTP_HEADER_TRANSFER_ENCODING);
    if (header != NULL) {
        m_transfer_encoding_ = header->get_value();
    }

    return YETI_SUCCESS;
}

YETI_Result HttpEntity::set_content_length(YETI_LargeSize length)
{
    m_content_length_ = length;
    m_content_length_is_known_ = true;
    return YETI_SUCCESS;
}

YETI_Result HttpEntity::set_content_type(const char * type)
{
    m_content_type_ = type;
    return YETI_SUCCESS;
}

YETI_Result HttpEntity::set_content_encoding(const char * encoding)
{
    m_content_encoding_ = encoding;
    return YETI_SUCCESS;
}

YETI_Result HttpEntity::set_transfer_encoding(const char * encoding)
{
    m_transfer_encoding_ = encoding;
    return YETI_SUCCESS;
}

HttpMessage::HttpMessage(const char * protocol)
: m_protocol_(protocol)
, m_entity_(NULL)
{
}

HttpMessage::~HttpMessage()
{
    delete m_entity_;
}

YETI_Result HttpMessage::set_entity(HttpEntity * entity)
{
    if (entity != m_entity_) {
        delete m_entity_;
        m_entity_ = entity;
    }
    return YETI_SUCCESS;
}

YETI_Result HttpMessage::parse_headers(BufferedInputStream & stream)
{
    return m_headers_.parse(stream);
}

YETI_Result HttpRequest::parse(BufferedInputStream & stream,
                               const SocketAddress * endpoint,
                               HttpRequest *& request)
{
    // default return value
    request = NULL;
skip_first_empty_line:
    // read the request line
    String line;
    YETI_CHECK_FINER(stream.read_line(line, YETI_HTTP_PROTOCOL_MAX_LINE_LENGTH));
    YETI_LOG_FINEST_1("http request: %s", line.get_chars());
    // when using keep-alive connections, clients such as XBox 360
    // incorrectly send a few empty lines as body for GET requests
    // so we try to skip them until we find something to parse
    if (line.get_length() == 0) goto skip_first_empty_line;
    if (line.get_length() == 0) goto skip_first_empty_line;

    // check the request line
    int first_space = line.find(' ');
    if (first_space < 0) {
        YETI_LOG_FINE_1("http request: %s", line.get_chars());
        return YETI_ERROR_HTTP_INVALID_REQUEST_LINE;
    }
    int second_space = line.find(' ', first_space + 1);
    if (second_space < 0) {
        YETI_LOG_FINE_1("http request: %s", line.get_chars());
        return YETI_ERROR_HTTP_INVALID_REQUEST_LINE;
    }

    // parse the request line
    String method = line.sub_string(0, first_space);
    String uri = line.sub_string(first_space + 1, second_space - first_space - 1);
    String protocol = line.sub_string(second_space + 1);

    // create a request
    bool proxy_style_request = false;
    if (uri.starts_with("http://", true)) {
        // proxy-style request with absolute URI
        request = new HttpRequest(uri, method, protocol);
        proxy_style_request = true;
    } else {
        // normal absolute path request
        request = new HttpRequest("http:", method, protocol);
    }

    // parse headers
    YETI_Result result = request->parse_headers(stream);
    if (YETI_FAILED(result)) {
        delete request;
        request = NULL;
        return result;
    }

    // update the URL
    if (!proxy_style_request) {
        request->m_url_.set_scheme("http");
        request->m_url_.parse_pathplus(uri);
        request->m_url_.set_port(YETI_HTTP_DEFAULT_PORT);

        // check for a Host: header
        HttpHeader * host_header = request->get_headers().get_header(YETI_HTTP_HEADER_HOST);
        if (host_header) {
            request->m_url_.set_host(host_header->get_value());
            // host sometimes doesn't contain port
            if (endpoint) {
                request->m_url_.set_port(endpoint->get_port());
            }
        } else {
            // use the endpoint as the host
            if (endpoint) {
                request->m_url_.set_host(endpoint->to_string());
            } else {
                // use defaults
                request->m_url_.set_host("localhost");
            }
        }
    }

    return YETI_SUCCESS;
}

HttpRequest::HttpRequest(const HttpUrl & url,
                         const char * method,
                         const char * protocol /*= YETI_HTTP_PROTOCOL_1_0*/)
                         : HttpMessage(protocol)
                         , m_url_(url)
                         , m_method_(method)
{
}

HttpRequest::HttpRequest(const char * url,
                         const char * mothod,
                         const char * protocol/* = YETI_HTTP_PROTOCOL_1_0*/)
                         : HttpMessage(protocol)
                         , m_url_(url)
                         , m_method_(m_method_)
{
}

HttpRequest::~HttpRequest()
{
}

YETI_Result HttpRequest::set_url(const char * url)
{
    m_url_ = url;
    return YETI_SUCCESS;
}

YETI_Result HttpRequest::set_url(const HttpUrl & url)
{
    m_url_ = url;
    return YETI_SUCCESS;
}

YETI_Result HttpRequest::emit(OutputStream & stream, bool use_proxy /*= false*/) const
{
    stream.write_string(m_method_);
    stream.write_fully(" ", 1);
    if (use_proxy) {
        stream.write_string(m_url_.to_string(false));
    } else {
        stream.write_string(m_url_.to_request_string());
    }
    stream.write_fully(" ", 1);
    stream.write_string(m_protocol_);
    stream.write_fully(YETI_HTTP_LINE_TERMINATOR, 2);

    m_headers_.emit(stream);
    stream.write_fully(YETI_HTTP_LINE_TERMINATOR, 2);
    return YETI_SUCCESS;
}

YETI_Result HttpResponse::parse(BufferedInputStream & stream,
                                HttpResponse *& response)
{
    response = NULL;
    String line;
    YETI_CHECK_FINE(stream.read_line(line, YETI_HTTP_PROTOCOL_MAX_LINE_LENGTH));
    /*if (YETI_FAILED(res)) {
    if (res != YETI_ERROR_TIMEOUT && res != YETI_ERROR_EOS) YETI_CHECK_WARNING(res);
    return res;
    }*/

    YETI_LOG_FINE_1("http response: %s", line.get_chars());

    int first_space = line.find(' ');
    if (first_space < 1) return YETI_ERROR_HTTP_INVALID_RESPONSE_LINE;
    int second_space =  line.find(' ', first_space + 1);
    if (second_space < 0) {
        if (line.get_length() != 12) {
            return YETI_ERROR_HTTP_INVALID_RESPONSE_LINE;
        }
    } else if (second_space != 4) {
        return YETI_ERROR_HTTP_INVALID_RESPONSE_LINE;
    }

    String protocol = line.sub_string(0, first_space);
    String status_code = line.sub_string(first_space + 1, 3);
    String reason_phrase = line.sub_string(first_space + 1 + 3 + 1,
        line.get_length() - (first_space + 1 + 3 + 1));

    YETI_UInt32 status_code_int = 0;
    status_code.to_integer32(status_code_int);
    response = new HttpResponse(status_code_int, reason_phrase, protocol);

    YETI_Result result = response->parse_headers(stream);
    if (YETI_FAILED(result)) {
        delete response;
        response = NULL;
    }

    return result;
}

HttpResponse::HttpResponse(HttpStatusCode status_code,
                           const char * reason_phrase,
                           const char * protocol /*= YETI_HTTP_PROTOCOL_1_0*/)
                           : HttpMessage(protocol)
                           , m_status_code_(status_code)
                           , m_reason_phrase_(reason_phrase)
{
}

HttpResponse::~HttpResponse()
{
}

YETI_Result HttpResponse::set_status(HttpStatusCode status_code,
                                     const char * reason_phrase,
                                     const char * protocol /*= NULL*/)
{
    m_status_code_ = status_code;
    m_reason_phrase_ = reason_phrase;
    if (protocol) m_protocol_ = protocol;
    return YETI_SUCCESS;
}

YETI_Result HttpResponse::set_protocol(const char * protocol)
{
    m_protocol_ = protocol;
    return YETI_SUCCESS;
}

YETI_Result HttpResponse::emit(OutputStream & stream) const
{
    stream.write_string(m_protocol_);
    stream.write_fully(" ", 1);
    stream.write_string(String::from_integer(m_status_code_));
    stream.write_fully(" ", 1);
    stream.write_string(m_reason_phrase_);
    stream.write_fully(YETI_HTTP_LINE_TERMINATOR, 2);

    m_headers_.emit(stream);

    stream.write_fully(YETI_HTTP_LINE_TERMINATOR, 2);

    return YETI_SUCCESS;
}

class HttpSimpleConnection : public HttpClient::Connection
{
public:
    virtual ~HttpSimpleConnection() {
        HttpClient::ConnectionCanceller::untrack(this);
    }

    virtual InputStreamReference & get_input_stream() {
        return m_inputstream_;
    }

    virtual OutputStreamReference & get_output_stream() {
        return m_outputstream_;
    }

    virtual YETI_Result get_info(SocketInfo & info) {
        return m_socket_->get_info(info);
    }

    virtual YETI_Result abort() {
        return m_socket_->cancel();
    }

    SocketReference         m_socket_;
    InputStreamReference    m_inputstream_;
    OutputStreamReference   m_outputstream_;
};

class HttpTcpConnector : public HttpClient::Connector
{
    virtual YETI_Result connect(const HttpUrl & url,
        HttpClient & client,
        const HttpProxyAddress * proxy,
        bool reuse,
        HttpClient::Connection *& connection);
};

YETI_Result HttpTcpConnector::connect(const HttpUrl & url,
                                      HttpClient & client,
                                      const HttpProxyAddress * proxy,
                                      bool reuse, HttpClient::Connection *& connection)
{
    connection = NULL;
    const char * server_hostname;
    YETI_UInt16 server_port;
    if (proxy) {
        server_hostname = (const char *)proxy->get_hostname();
        server_port = proxy->get_port();
    } else {
        server_hostname = (const char *)url.get_host();
        server_port = url.get_port();
    }

    IpAddress address;
    YETI_CHECK_FINE(address.resolve_name(server_hostname, client.get_config().m_name_resolver_timeout_));
    YETI_LOG_FINE_2("TCP connector will connect to %s : %d", server_hostname, server_port);
    TcpClientSocket * tcp_socket = new TcpClientSocket();
    SocketReference socket(tcp_socket, true);
    tcp_socket->set_read_timeout(client.get_config().m_io_timeout_);
    tcp_socket->set_write_timeout(client.get_config().m_io_timeout_);
    SocketAddress socket_address(address, server_port);
    YETI_CHECK_FINE(tcp_socket->connect(socket_address, client.get_config().m_connection_timeout_));

    HttpSimpleConnection * _connection = new HttpSimpleConnection();
    _connection->m_socket_ = socket;
    connection = _connection;
    tcp_socket->get_input_stream(_connection->m_inputstream_);
    tcp_socket->get_output_stream(_connection->m_outputstream_);

    return YETI_SUCCESS;
}

class HttpEnvProxySelector : public HttpProxySelector
{
public:
    class Cleanner {
        static Cleanner automatic_cleaner;
        ~Cleanner() {
            if (m_instance_) {
                delete m_instance_;
                m_instance_ = NULL;
            }
        }
    };

    static HttpEnvProxySelector * get_instance();

    YETI_Result get_proxy_for_url(const HttpUrl & url, HttpProxyAddress & proxy);

private:
    static HttpEnvProxySelector * m_instance_;
    static void _parse_proxy_env(const String & env, HttpProxyAddress & proxy);

    HttpProxyAddress m_http_proxy_;
    HttpProxyAddress m_https_proxy_;
    List<String>     m_no_proxy_;
    HttpProxyAddress m_all_proxy_;
};

HttpEnvProxySelector * HttpEnvProxySelector::m_instance_ = NULL;
HttpEnvProxySelector::Cleanner HttpEnvProxySelector::Cleanner::automatic_cleaner;

HttpEnvProxySelector * HttpEnvProxySelector::get_instance()
{
    if (m_instance_) return m_instance_;
    SingletonLock::get_instance().lock();
    if (m_instance_ == NULL) {
        m_instance_ = new HttpEnvProxySelector();
        String http_proxy;
        Environment::get("http_proxy", http_proxy);
        _parse_proxy_env(http_proxy, m_instance_->m_http_proxy_);
        YETI_LOG_FINE_2("http_proxy: %s:%d", m_instance_->m_http_proxy_.get_hostname().get_chars(), m_instance_->m_http_proxy_.get_port());

        String https_proxy;
        if (YETI_FAILED(Environment::get("HTTPS_PROXY", https_proxy))) {
            Environment::get("https_proxy", https_proxy);
        }
        _parse_proxy_env(https_proxy, m_instance_->m_https_proxy_);
        YETI_LOG_FINE_2("https_proxy: %s:%d", m_instance_->m_https_proxy_.get_hostname().get_chars(), m_instance_->m_https_proxy_.get_port());

        String all_proxy;
        if (YETI_FAILED(Environment::get("ALL_PROXY", all_proxy))) {
            Environment::get("all_proxy", all_proxy);
        }
        _parse_proxy_env(all_proxy, m_instance_->m_all_proxy_);
        YETI_LOG_FINE_2("all_proxy: %s:%d", m_instance_->m_all_proxy_.get_hostname().get_chars(), m_instance_->m_all_proxy_.get_port());

        String no_proxy;
        if (YETI_FAILED(Environment::get("NO_PROXY", no_proxy))) {
            Environment::get("no_proxy", no_proxy);
        }
        if (no_proxy.get_length()) {
            m_instance_->m_no_proxy_ = no_proxy.split(",");
        }
    }
    SingletonLock::get_instance().unlock();
    return m_instance_;
}

void HttpEnvProxySelector::_parse_proxy_env(const String & env, HttpProxyAddress & proxy)
{
    if (env.get_length() == 0) return;
    String proxy_spec;
    if (env.find("://") >= 0) {
        proxy_spec = env;
    } else {
        proxy_spec = "http://" + env;
    }
    Url url(proxy_spec);
    proxy.set_hostname(url.get_host());
    proxy.set_port(url.get_port());
}

YETI_Result HttpEnvProxySelector::get_proxy_for_url(const HttpUrl & url, HttpProxyAddress & proxy)
{
    HttpProxyAddress * protocol_proxy = NULL;
    switch (url.get_schemeid()) {
case Uri::SCHEME_ID_HTTP:
    protocol_proxy = &m_http_proxy_;
    break;

case Uri::SCHEME_ID_HTTPS:
    protocol_proxy = &m_https_proxy_;
    break;

default:
    return YETI_ERROR_HTTP_NO_PROXY;
    }

    if (m_no_proxy_.get_item_count()) {
        for (List<String>::iterator i = m_no_proxy_.get_first_item();
            i;
            ++i) {
                if ((*i) == "*") {
                    return YETI_ERROR_HTTP_NO_PROXY;
                }
                if (url.get_host().ends_with(*i, true)) {
                    if (url.get_host().get_length() == (*i).get_length()) {
                        return YETI_ERROR_HTTP_NO_PROXY;
                    }
                    if (url.get_host().get_chars()[url.get_host().get_length() - (*i).get_length() - 1] == '.') {
                        return YETI_ERROR_HTTP_NO_PROXY;
                    }
                }
        }
    }

    if (protocol_proxy->get_hostname().get_length()) {
        proxy = *protocol_proxy;
        return YETI_SUCCESS;
    }

    proxy = m_all_proxy_;
    return proxy.get_hostname().get_length() ? YETI_SUCCESS : YETI_ERROR_HTTP_NO_PROXY;
}

NAMEEND
