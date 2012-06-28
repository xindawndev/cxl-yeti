#include "Common.h"

USINGNAMESPACE2;
const char * g_buffer = "POST /volume?volume=0.062500 HTTP/1.1\r\n""Content-Length: 0\r\n""User-Agent: MediaControl/1.0\r\n\r\n";

int test_httpparser()
{
    HttpParser parser;
    HttpParser::status_t status;

    while (1) {
        // read bytes from socket into buffer, break on error
        status = parser.add_bytes( g_buffer, strlen(g_buffer) );
        if ( status != HttpParser::Incomplete ) break;
    }

    if ( status == HttpParser::Done ) {
        // parse fully formed http message.
        std::cout << "Method : " << parser.get_method() << std::endl;
        std::cout << "Uri : " << parser.get_uri() << std::endl;
        std::cout << "Content-Length : " << parser.get_content_length() << std::endl;
        std::cout << "User-Agent : " << parser.get_value("User-Agent") << std::endl;
    }
    return 0;
}

int httpparser_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_httpparser();
    return 0;
}

static TestRegister test("httpparser_test", httpparser_test);
