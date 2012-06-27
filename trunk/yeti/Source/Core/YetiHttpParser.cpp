#include "YetiHttpParser.h"
#include "YetiUtil.h"

NAMEBEG

HttpParser::HttpParser() :
m_header_start_(0),
m_body_start_(0),
m_parsed_to_( 0 ),
m_state_( 0 ),
m_key_index_(0),
m_value_index_(0),
m_content_length_(0),
m_content_start_(0),
m_uri_index_(0),
m_status_( Incomplete )
{
}

HttpParser::~HttpParser()
{
}

void HttpParser::_parse_header()
{
    // run the fsm.
    const int  CR = 13;
    const int  LF = 10;
    const int  ANY = 256;

    enum Action {
        // make lower case
        LOWER = 0x1,

        // convert current character to null.
        NULLIFY = 0x2,

        // set the header index to the current position
        SET_HEADER_START = 0x4,

        // set the key index to the current position
        SET_KEY = 0x8,

        // set value index to the current position.
        SET_VALUE = 0x10,

        // store current key/value pair.
        STORE_KEY_VALUE = 0x20,

        // sets content start to current position + 1
        SET_CONTENT_START = 0x40
    };

    static const struct FSM {
        State curState;
        int c;
        State nextState;
        unsigned actions;
    } fsm[] = {
        { p_request_line,         CR, p_request_line_cr,     NULLIFY                            },
        { p_request_line,        ANY, p_request_line,        0                                  },
        { p_request_line_cr,      LF, p_request_line_crlf,   0                                  },
        { p_request_line_crlf,    CR, p_request_line_crlfcr, 0                                  },
        { p_request_line_crlf,   ANY, p_key,                 SET_HEADER_START | SET_KEY | LOWER },
        { p_request_line_crlfcr,  LF, p_content,             SET_CONTENT_START                  },
        { p_key,                 ':', p_key_colon,           NULLIFY                            },
        { p_key,                 ANY, p_key,                 LOWER                              },
        { p_key_colon,           ' ', p_key_colon_sp,        0                                  },
        { p_key_colon_sp,        ANY, p_value,               SET_VALUE                          },
        { p_value,                CR, p_value_cr,            NULLIFY | STORE_KEY_VALUE          },
        { p_value,               ANY, p_value,               0                                  },
        { p_value_cr,             LF, p_value_crlf,          0                                  },
        { p_value_crlf,           CR, p_value_crlfcr,        0                                  },
        { p_value_crlf,          ANY, p_key,                 SET_KEY | LOWER                    },
        { p_value_crlfcr,         LF, p_content,             SET_CONTENT_START                  },
        { p_error,               ANY, p_error,               0                                  }
    };

    for( unsigned i = m_parsed_to_; i < m_data_.get_length(); ++i) {
        char c = m_data_[i];
        State nextState = p_error;

        for ( unsigned d = 0; d < sizeof(fsm) / sizeof(FSM); ++d ) {
            if ( fsm[d].curState == m_state_ && 
                ( c == fsm[d].c || fsm[d].c == ANY ) ) {

                    nextState = fsm[d].nextState;

                    if ( fsm[d].actions & LOWER ) {
                        m_data_[i] = lowercase( m_data_[i] );
                    }

                    if ( fsm[d].actions & NULLIFY ) {
                        m_data_[i] = 0;
                    }

                    if ( fsm[d].actions & SET_HEADER_START ) {
                        m_header_start_ = i;
                    }

                    if ( fsm[d].actions & SET_KEY ) {
                        m_key_index_ = i;
                    }

                    if ( fsm[d].actions & SET_VALUE ) {
                        m_value_index_ = i;
                    }

                    if ( fsm[d].actions & SET_CONTENT_START ) {
                        m_content_start_ = i + 1;
                    }

                    if ( fsm[d].actions & STORE_KEY_VALUE ) {
                        // store position of first character of key.
                        m_keys_.add( m_key_index_ );
                    }

                    break;
            }
        }

        m_state_ = nextState;

        if ( m_state_ == p_content ) {
            String str = get_value("content-length");
            if ( !str.is_empty() ) {
                str.to_integer(m_content_length_);
            }
            break;
        }
    }

    m_parsed_to_ = m_data_.get_length();
}

bool HttpParser::_parse_request_line()
{
    size_t sp1;
    size_t sp2;

    sp1 = m_data_.find( ' ', 0 );
    if ( sp1 == String::npos ) return false;
    sp2 = m_data_.find( ' ', sp1 + 1 );
    if ( sp2 == String::npos ) return false;

    m_data_[sp1] = 0;
    m_data_[sp2] = 0;
    m_uri_index_ = sp1 + 1;
    return true;
}

HttpParser::status_t HttpParser::add_bytes( const char* bytes, unsigned len )
{
    if ( m_status_ != Incomplete ) {
        return m_status_;
    }

    // append the bytes to data.
    m_data_.append( bytes, len );

    if ( m_state_ < p_content ) {
        _parse_header();
    }

    if ( m_state_ == p_error ) {
        m_status_ = Error;
    } else if ( m_state_ == p_content ) {
        if ( m_content_length_ == 0 || m_data_.get_length() - m_content_start_ >= m_content_length_ ) {
            if ( _parse_request_line() ) {
                m_status_ = Done;
            } else {
                m_status_ = Error;
            }
        }
    }

    return m_status_;
}

const char* HttpParser::get_method()
{
    return &m_data_[0];
}

const char* HttpParser::get_uri()
{
    return &m_data_[m_uri_index_];
}

const char* HttpParser::get_query_string()
{
    const char* pos = get_uri();
    while( *pos ) {
        if ( *pos == '?' ) {
            pos++;
            break;
        }
        pos++;
    }
    return pos;
}

const char* HttpParser::get_body()
{
    if ( m_content_length_ > 0 ) {
        return &m_data_[m_content_start_];
    } else  {
        return NULL;
    }
}

// key should be in lower case.
const char* HttpParser::get_value( const char* key )
{
    for( IntArray::iterator iter = m_keys_.get_first_item();
        iter != m_keys_.get_last_item(); ++iter  )
    {
        unsigned index = *iter;
        if ( StringEqual( &m_data_[index], key ) == 0 ) {
            return &m_data_[index + strlen(key) + 2];
        }
    }

    return NULL;
}

unsigned HttpParser::get_content_length()
{
    return m_content_length_;
}

NAMEEND
