#ifndef _CXL_YETI_HTTPPARSER_H_
#define _CXL_YETI_HTTPPARSER_H_

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiResults.h"
#include "YetiArray.h"
#include "YetiString.h"

NAMEBEG

// Example usage:
// 
//    HttpParser parser;
//    HttpParser::status_t status;
//
//    for( ;; ) {
//        // read bytes from socket into buffer, break on error
//        status = parser.add_bytes( buffer, length );
//        if ( status != HttpParser::Incomplete ) break;
//    }
//
//    if ( status == HttpParser::Done ) {
//        // parse fully formed http message.
//    }

class HttpParser
{
public:
    HttpParser();
    ~HttpParser();

    enum status_t {
        Done,
        Error,
        Incomplete
    };

    status_t add_bytes( const char* bytes, YETI_Size len );

    const char* get_method();
    const char* get_uri();
    const char* get_query_string();
    const char* get_body();
    // key should be in lower case when looking up.
    const char* get_value( const char* key );
    YETI_Size get_content_length();

private:
    void _parse_header();
    bool _parse_request_line();

    String m_data_;
    unsigned m_header_start_;
    unsigned m_body_start_;
    unsigned m_parsed_to_;
    int m_state_;
    unsigned m_key_index_;
    unsigned m_value_index_;
    YETI_Size m_content_length_;
    unsigned m_content_start_;
    unsigned m_uri_index_;
    
    typedef Array<unsigned> IntArray;
    IntArray m_keys_;

    enum State {
        p_request_line              = 0,
        p_request_line_cr           = 1,
        p_request_line_crlf         = 2,
        p_request_line_crlfcr       = 3,
        p_key                       = 4,
        p_key_colon                 = 5,
        p_key_colon_sp              = 6,
        p_value                     = 7,
        p_value_cr                  = 8,
        p_value_crlf                = 9,
        p_value_crlfcr              = 10,
        p_content                   = 11, // here we are done parsing the header.
        p_error                     = 12  // here an error has occurred and the parse failed.
    };

    status_t m_status_;
};

NAMEEND
 
#endif // _CXL_YETI_HTTPPARSER_H_
