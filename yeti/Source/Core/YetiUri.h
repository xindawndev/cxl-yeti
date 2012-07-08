#ifndef _CXL_YETI_URI_H_
#define _CXL_YETI_URI_H_

#include "YetiString.h"
#include "YetiList.h"

NAMEBEG

const YETI_UInt16 YETI_URL_INVALID_PORT       = 0;
const YETI_UInt16 YETI_URL_DEFAULT_HTTP_PORT  = 80;
const YETI_UInt16 YETI_URL_DEFAULT_HTTPS_PORT = 443;

class Uri 
{
public:
    typedef enum{
        SCHEME_ID_UNKNOWN,
        SCHEME_ID_HTTP,
        SCHEME_ID_HTTPS
    } SchemeId;

    static const char * const PathCharsToEncode;
    static const char * const QueryCharsToEncode;
    static const char * const FragmentCharsToEncode;
    static const char * const UnsafeCharsToEncode;

    static String percent_encode(const char * str, const char * chars, bool encode_percents = true);
    static String percent_decode(const char * str);
    static SchemeId parse_scheme(const String & scheme);

    Uri() : m_schemeid_(SCHEME_ID_UNKNOWN) {}
    virtual ~Uri() {}

    const String & get_scheme() const {
        return m_scheme_;
    }

    void set_scheme(const char * scheme);
    YETI_Result set_scheme_from_uri(const char * uri);
    SchemeId get_schemeid() const {
        return m_schemeid_;
    }

protected:
    String m_scheme_;
    SchemeId m_schemeid_;
};

class UrlQuery
{
public:
    static String url_encode(const char * str, bool encode_percents = true);
    static String url_decode(const char * str);

    struct Field {
        Field(const char * name, const char * value, bool encoded);
        String m_name_;
        String m_value_;
    };

    UrlQuery() {}
    UrlQuery(const char * query);

    List<Field> & get_fields() { return m_fields_; }
    YETI_Result parse(const char * query);
    YETI_Result set_field(const char * name, const char * value, bool encoded = false);
    YETI_Result add_field(const char * name, const char * value, bool encoded = false);
    const char * get_field(const char * name);
    String to_string();

private:
    List< Field > m_fields_;
};

class Url : public Uri
{
public:
    Url();
    Url(const char * url, YETI_UInt16 default_port = 0);
    Url(const char * scheme,
        const char * host,
        YETI_UInt16 port,
        const char * path,
        const char * query = NULL,
        const char * fragment = NULL);

    YETI_Result parse(const char * url, YETI_UInt16 default_port = 0);
    YETI_Result parse_pathplus(const char * path_plus);
    const String & get_host() const { return m_host_; }
    YETI_UInt16 get_port() const { return m_port_; }
    const String & get_path() const { return m_path_; }
    String get_path(bool decoded) const { return decoded ? Uri::percent_decode(m_path_) : m_path_; }
    const String & get_query() const { return m_query_; }
    const String & get_fragment() const { return m_fragment_; }
    virtual bool is_valid() const;
    void reset();
    bool has_query() const { return m_hasquery_; }
    bool has_fragment() const { return m_hasfragment_; }

    YETI_Result set_host(const char * host);
    YETI_Result set_port(YETI_UInt16 port);
    YETI_Result set_path(const char * path, bool encoded = false);
    YETI_Result set_query(const char * query, bool encoded = false);
    YETI_Result set_fragment(const char * fragment, bool encoded = false);

    virtual String to_request_string(bool with_fragment = false) const;
    virtual String to_string_with_default_port(YETI_UInt16 default_port, bool with_fragment = true) const;
    virtual String to_string(bool with_fragment = true) const;
protected:
    String m_host_;
    YETI_UInt16 m_port_;
    String m_path_;
    bool m_hasquery_;
    String m_query_;
    bool m_hasfragment_;
    String m_fragment_;
};
NAMEEND

#endif // _CXL_YETI_URI_H_
