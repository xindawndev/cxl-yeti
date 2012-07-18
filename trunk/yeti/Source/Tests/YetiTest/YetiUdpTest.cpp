#include "Common.h"

USINGNAMESPACE2;

int my_receive()
{
    printf("==== Receive\n");
    UdpSocket receiver;
    DataBuffer buffer(4096);
    buffer.set_data_size(4096);
    YETI_Result result = receiver.bind(SocketAddress(IpAddress::Any, 9123));
    if (YETI_FAILED(result)) {
        fprintf(stderr, "bind() failed (%d)\n", result);
        return result;
    }

    SocketAddress address;
    result = receiver.receive(buffer, &address);
    if (YETI_FAILED(result)) {
        fprintf(stderr, "receive() failed (%d)\n", result);
        return result;
    }

    String addr_string = address.get_ipaddress().to_string();

    printf("received packet, size=%d, from %s:%d\n",
        (int)buffer.get_data_size(),
        (const char*)addr_string,
        (int)address.get_port());
    return 0;
}

int my_send()
{
    printf("==== Send\n");

    char * msg = "hello";
    UdpSocket sender;
    DataBuffer buffer(1024);
    buffer.set_data_size(1024);
    buffer.set_data((YETI_Byte *)msg, strlen(msg));
    IpAddress address;
    address.resolve_name("localhost");
    SocketAddress socket_address(address, 9123);
    YETI_Result result = sender.send(buffer, &socket_address);
    if (YETI_FAILED(result)) {
        fprintf(stderr, "send() failed (%d)\n", result);
        return result;
    }
    return 0;
}

int udp_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    if (args.size() >= 3) {
        if (StringEqual(args[2].c_str(), "send")) {
            my_send();
        } else {
            my_receive();
        }
    }
    return 0;
}

static TestRegister test("udp_test", udp_test);
