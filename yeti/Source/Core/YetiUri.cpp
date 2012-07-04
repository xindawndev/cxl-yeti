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
    return YETI_SUCCESS;
}

NAMEEND
