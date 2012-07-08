#include "YetiUri.h"
#include "YetiUtil.h"
#include "YetiResults.h"

NAMEBEG

Uri::SchemeId Uri::parse_scheme(const String & scheme)
{
    if (scheme == "http") {
        return SCHEME_ID_HTTP;
    } else if (scheme == "https") {
        return SCHEME_ID_HTTPS;
    } else {
        return SCHEME_ID_UNKNOWN;   
    }
}

YETI_Result Uri::set_scheme_from_uri(const char * uri)
{
    const char * start = uri;
    char c;
    while ((c = *uri++)) {
        if (c == ':') {
            m_scheme_.assign(start, (YETI_Size)(uri - start - 1));
            m_scheme_.make_lowercase();
            m_schemeid_ = parse_scheme(m_scheme_);
        } else if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            (c == '+')             ||
            (c == '.')             ||
            (c == '-')) {
            continue;
        } else {
            break;
        }
    }
    return YETI_ERROR_INVALID_SYNTAX;
}

/*----------------------------------------------------------------------
Appendix A.  Collected ABNF for URI

   URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]

   hier-part     = "//" authority path-abempty
                 / path-absolute
                 / path-rootless
                 / path-empty

   URI-reference = URI / relative-ref

   absolute-URI  = scheme ":" hier-part [ "?" query ]

   relative-ref  = relative-part [ "?" query ] [ "#" fragment ]

   relative-part = "//" authority path-abempty
                 / path-absolute
                 / path-noscheme
                 / path-empty

   scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )

   authority     = [ userinfo "@" ] host [ ":" port ]
   userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
   host          = IP-literal / IPv4address / reg-name
   port          = *DIGIT

   IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"

   IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )

   IPv6address   =                            6( h16 ":" ) ls32
                 /                       "::" 5( h16 ":" ) ls32
                 / [               h16 ] "::" 4( h16 ":" ) ls32
                 / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
                 / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
                 / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
                 / [ *4( h16 ":" ) h16 ] "::"              ls32
                 / [ *5( h16 ":" ) h16 ] "::"              h16
                 / [ *6( h16 ":" ) h16 ] "::"

   h16           = 1*4HEXDIG
   ls32          = ( h16 ":" h16 ) / IPv4address
   IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
   dec-octet     = DIGIT                 ; 0-9
                 / %x31-39 DIGIT         ; 10-99
                 / "1" 2DIGIT            ; 100-199
                 / "2" %x30-34 DIGIT     ; 200-249
                 / "25" %x30-35          ; 250-255

   reg-name      = *( unreserved / pct-encoded / sub-delims )

   path          = path-abempty    ; begins with "/" or is empty
                 / path-absolute   ; begins with "/" but not "//"
                 / path-noscheme   ; begins with a non-colon segment
                 / path-rootless   ; begins with a segment
                 / path-empty      ; zero characters

   path-abempty  = *( "/" segment )
   path-absolute = "/" [ segment-nz *( "/" segment ) ]
   path-noscheme = segment-nz-nc *( "/" segment )
   path-rootless = segment-nz *( "/" segment )
   path-empty    = 0<pchar>

   segment       = *pchar
   segment-nz    = 1*pchar
   segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
                 ; non-zero-length segment without any colon ":"

   pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"

   query         = *( pchar / "/" / "?" )

   fragment      = *( pchar / "/" / "?" )

   pct-encoded   = "%" HEXDIG HEXDIG

   unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
   reserved      = gen-delims / sub-delims
   gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
   sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
                 / "*" / "+" / "," / ";" / "="

---------------------------------------------------------------------*/

#define YETI_URI_ALWAYS_ENCODE " !\"<>\\^`{|}"

const char * const Uri::PathCharsToEncode = YETI_URI_ALWAYS_ENCODE "?#[]";

const char * const Uri::QueryCharsToEncode = YETI_URI_ALWAYS_ENCODE "#[]";

const char * const Uri::FragmentCharsToEncode = YETI_URI_ALWAYS_ENCODE "[]";

const char * const Uri::UnsafeCharsToEncode = YETI_URI_ALWAYS_ENCODE;

String Uri::percent_encode(const char * str, const char * chars, bool encode_percents /* = true */)
{
    String encoded;
    if (str == NULL) return encoded;
    encoded.reserve(StringLength(str));
    char escaped[3];
    escaped[0] = '%';
    while (unsigned char c = *str++) {
        bool encode = false;
        if (encode_percents && c == '%') {
            encode = true;
        } else if (c < ' ' || c > '~') {
            encode = true;
        } else {
            const char * match = chars;
            while (*match) {
                if (c == *match) {
                    encode = true;
                    break;
                }
                ++match;
            }
        }
        if (encode) {
            byte_to_hex(c, &escaped[1], true);
            encoded.append(escaped, 3);
        } else {
            encoded += c;
        }
    }
    return encoded;
}

String Uri::percent_decode(const char * str)
{
    String decoded;
    if (str == NULL) return decoded;
    while (unsigned char c = *str++) {
        if (c == '%') {
            unsigned char unescaped;
            if (YETI_SUCCEEDED(hex_to_byte(str, unescaped))) {
                decoded += unescaped;
                str += 2;
            } else {
                decoded += c;
            }
        } else {
            decoded += c;
        }
    }
    return decoded;
}

UrlQuery::UrlQuery(const char * query)
{
    parse(query);
}

String UrlQuery::url_encode(const char * str, bool encode_percents /* = true */)
{
    String encoded = Uri::percent_encode(str,
        ";/?:@&=+$,"
        "\"#<>\\^`{|}",
        encode_percents);
    encoded.replace(' ', '+');
    return encoded;
}

String UrlQuery::url_decode(const char * str)
{
    String decoded = Uri::percent_decode(str);
    decoded.replace('+', ' ');
    return decoded;
}

UrlQuery::Field::Field(const char * name, const char * value, bool encoded)
{
    if (encoded) {
        m_name_ = name;
        m_value_ = value;
    } else {
        m_name_ = url_encode(name);
        m_value_ = url_encode(value);
    }
}

String UrlQuery::to_string()
{
    String encoded;
    bool separator = false;
    for (List<Field>::iterator it = m_fields_.get_first_item(); it; ++it) {
        Field & field = *it;
        if (separator) encoded += "&";
        separator = true;
        encoded += field.m_name_;
        encoded += "=";
        encoded += field.m_value_;
    }

    return encoded;
}

YETI_Result UrlQuery::parse(const char * query)
{
    const char * cursor = query;
    String name;
    String value;
    bool in_name = true;
    do 
    {
        if (*cursor == '\0' || *cursor == '&') {
            if (!name.is_empty()) {
                add_field(name, value, true);
            }
            name.set_length(0);
            value.set_length(0);
            in_name = true;
        } else if (*cursor == '=' && in_name) {
            in_name = false;
        } else {
            if (in_name) {
                name += *cursor;
            } else {
                value += *cursor;
            }
        }
    } while (*cursor++);
    return YETI_SUCCESS;
}

YETI_Result UrlQuery::add_field(const char * name, const char * value, bool encoded /* = false */)
{
    return m_fields_.add(Field(name, value, encoded));
}

YETI_Result UrlQuery::set_field(const char * name, const char * value, bool encoded /* = false */)
{
    String ename;
    if (encoded) {
        ename = name;
    } else {
        ename = url_encode(name);
    }

    for (List<Field>::iterator it = m_fields_.get_first_item(); it; ++it) {
        Field & field = *it;
        if (field.m_name_ == ename) {
            if (encoded) {
                field.m_value_ = true;
            } else {
                field.m_value_ = url_encode(value);
            }
            return YETI_SUCCESS;
        }
    }

    return add_field(name, value, encoded);
}

const char * UrlQuery::get_field(const char * name)
{
    String ename = url_encode(name);
    for (List<Field>::iterator it = m_fields_.get_first_item(); it; ++it) {
        Field & field = *it;
        if (field.m_name_ == ename) return field.m_value_;
    }
    return NULL;
}

typedef enum {
    URL_PARSER_STATE_START,
    URL_PARSER_STATE_SCHEME,
    URL_PARSER_STATE_LEADING_SLASH,
    URL_PARSER_STATE_HOST,
    URL_PARSER_STATE_PORT,
    URL_PARSER_STATE_PATH,
    URL_PARSER_STATE_QUERY
} UrlParserState;

Url::Url()
: m_port_(YETI_URL_INVALID_PORT)
, m_path_("/")
, m_hasquery_(false)
, m_hasfragment_(false)
{
}

Url::Url(const char * url, YETI_UInt16 default_port/* = 0*/)
: m_port_(YETI_URL_INVALID_PORT)
, m_hasquery_(false)
, m_hasfragment_(false)
{
    if (YETI_FAILED(parse(url, default_port))) {
        reset();
    }
}

Url::Url(const char * scheme, const char * host, YETI_UInt16 port, const char * path, const char * query /* = NULL */, const char * fragment /* = NULL */)
: m_host_(host)
, m_port_(port)
, m_path_(path)
, m_hasquery_(query != NULL)
, m_query_(query)
, m_hasfragment_(fragment != NULL)
, m_fragment_(fragment)
{
    set_scheme(scheme);
}

YETI_Result Url::parse(const char * url, YETI_UInt16 default_port /* = 0 */)
{
    if (url == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    YETI_Result result = set_scheme_from_uri(url);
    if (YETI_FAILED(result)) return result;

    if (default_port) {
        m_port_ = default_port;
    } else {
        switch (m_schemeid_) {
            case SCHEME_ID_HTTP: m_port_ = YETI_URL_DEFAULT_HTTP_PORT; break;
            case SCHEME_ID_HTTPS: m_port_ = YETI_URL_DEFAULT_HTTPS_PORT; break;
            default: break;
        }
    }

    url += m_scheme_.get_length() + 1;
    UrlParserState state = URL_PARSER_STATE_START;
    const char * mark = url;
    char c;
    do
    {
        c = *url++;
        switch (state) {
            case URL_PARSER_STATE_START:
                if (c == '/') {
                    state = URL_PARSER_STATE_LEADING_SLASH;
                } else {
                    return YETI_ERROR_INVALID_SYNTAX;
                }
                break;

            case URL_PARSER_STATE_LEADING_SLASH:
                if (c == '/') {
                    state = URL_PARSER_STATE_HOST;
                    mark = url;
                } else {
                    return YETI_ERROR_INVALID_SYNTAX;
                }
                break;
            case URL_PARSER_STATE_HOST:
                if (c == ':' || c == '/' || c == '\0') {
                    m_host_.assign(mark, (YETI_Size)(url - 1 -mark));
                    if (c == ':') {
                        mark = url;
                        m_port_ = 0;
                        state = URL_PARSER_STATE_PORT;
                    } else {
                        mark = url - 1;
                        state = URL_PARSER_STATE_PATH;
                    }
                }
                break;
            case URL_PARSER_STATE_PORT:
                if (c >= '0' && c <= '9') {
                    unsigned int val = m_port_ * 10 + (c - '0');
                    if (val > 65535) {
                        m_port_ = YETI_URL_INVALID_PORT;
                        return YETI_ERROR_INVALID_SYNTAX;
                    }
                    m_port_ = val;
                } else if (c == '/' || c == '\0') {
                    mark = url - 1;
                    state = URL_PARSER_STATE_PATH;
                } else {
                    m_port_ = YETI_URL_INVALID_PORT;
                    return YETI_ERROR_INVALID_SYNTAX;
                }
                break;
            case URL_PARSER_STATE_PATH:
                if (*mark) {
                    return parse_pathplus(mark);
                }
                break;
            default:
                break;
        }
    } while (c);

    m_path_ = "/";

    return YETI_SUCCESS;
}

void Url::reset()
{
    m_host_.set_length(0);
    m_port_ = 0;
    m_path_.set_length(0);
    m_hasquery_ = false;
    m_query_.set_length(0);
    m_hasfragment_ = false;
    m_fragment_.set_length(0);
}

bool Url::is_valid() const
{
    switch (m_schemeid_) {
        case SCHEME_ID_HTTP:
        case SCHEME_ID_HTTPS:
            return m_port_ != YETI_URL_INVALID_PORT && !m_host_.is_empty();
            break;
        default:
            return !m_scheme_.is_empty();
    }
}

YETI_Result Url::set_host(const char * host)
{
    const char * port = host;
    while (*port && *port != ':') port++;
    if (*port) {
        m_host_.assign(host, (YETI_Size)(port - host));
        unsigned int port_number;
        if (YETI_SUCCEEDED(parse_integer(port + 1, port_number, false))) {
            m_port_ = (short)port_number;
        }
    } else {
        m_host_ = host;
    }
    return YETI_SUCCESS;
}

YETI_Result Url::set_port(YETI_UInt16 port)
{
    m_port_ = port;
    return YETI_SUCCESS;
}

YETI_Result Url::set_path(const char * path, bool encoded /* = false */)
{
    if (encoded) {
        m_path_ = path;
    } else {
        m_path_ = percent_encode(path, PathCharsToEncode);
    }

    return YETI_SUCCESS;
}

YETI_Result Url::parse_pathplus(const char * path_plus)
{
    if (path_plus == NULL) return YETI_ERROR_INVALID_PARAMETERS;

    m_path_.set_length(0);
    m_query_.set_length(0);
    m_fragment_.set_length(0);
    m_hasquery_ = false;
    m_hasfragment_ = false;

    UrlParserState state = URL_PARSER_STATE_START;
    const char * mark = path_plus;
    char c;
    do {
        c = *path_plus++;
        switch (state) {
            case URL_PARSER_STATE_PATH:
                if (c == '\0' || c == '?' || c == '#') {
                    if (path_plus -1 > mark) {
                        m_path_.append(mark, (YETI_Size)(path_plus - 1 - mark));
                    }
                    if (c == '?') {
                        m_hasquery_ = true;
                        state = URL_PARSER_STATE_QUERY;
                        mark = path_plus;
                    } else if (c == '#') {
                        m_hasfragment_ = true;
                        m_fragment_ = path_plus;
                        return YETI_SUCCESS;
                    }
                }
                break;
            case URL_PARSER_STATE_QUERY:
                if (c =='\0' || c == '#') {
                    m_query_.assign(mark, (YETI_Size)(path_plus - 1 - mark));
                    if (c == '#') {
                        m_hasfragment_ = true;
                        m_fragment_ = path_plus;
                    }
                    return YETI_SUCCESS;
                }
                break;
            default:
                break;
        }
    } while(c);
    return YETI_SUCCESS;
}

YETI_Result Url::set_query(const char * query, bool encoded /* = false */)
{
    if (encoded) {
        m_query_ = query;
    } else {
        m_query_ = percent_encode(query, QueryCharsToEncode);
    }
    m_hasquery_ = query != NULL;

    return YETI_SUCCESS;
}

YETI_Result Url::set_fragment(const char * fragment, bool encoded /* = false */)
{
    if (encoded) {
        m_fragment_ = fragment;
    } else {
        m_fragment_ = percent_encode(fragment, FragmentCharsToEncode);
    }
    m_hasfragment_ = fragment != NULL;

    return YETI_SUCCESS;
}

String Url::to_request_string(bool with_fragment /* = false */) const
{
    String result;
    YETI_Size length = m_path_.get_length() + 1;
    if (m_hasquery_) length += 1 + m_query_.get_length();
    if (with_fragment) length += 1 + m_fragment_.get_length();
    result.reserve(length);

    if (m_path_.is_empty()) {
        result += "/";
    } else {
        result += m_path_;
    }
    if (m_hasquery_) {
        result += "?";
        result += m_query_;
    }
    if (with_fragment && m_hasfragment_) {
        result += "#";
        result += m_fragment_;
    }

    return result;
}

String Url::to_string_with_default_port(YETI_UInt16 default_port, bool with_fragment /* = true */) const
{
    String result;
    String request = to_request_string(with_fragment);
    YETI_Size length = m_scheme_.get_length() + 3 + m_host_.get_length() + 6 + request.get_length();

    result.reserve(length);
    result += m_scheme_;
    result += "://";
    result += m_host_;
    if (m_port_ != default_port) {
        String port = String::from_integer(m_port_);
        result += ':';
        result += port;
    }

    result += request;
    return result;
}

String Url::to_string(bool with_fragment /* = true */) const
{
    return to_string_with_default_port(0, with_fragment);
}

NAMEEND
