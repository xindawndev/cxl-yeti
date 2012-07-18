#include "Common.h"

USINGNAMESPACE2;

typedef enum {
    CLIENT_TYPE_UNKNOWN,
    CLIENT_TYPE_UDP,
    CLIENT_TYPE_TCP
} ClientType;

static struct {
    bool verbose;
} Options;

void ping_print_useage_and_exit()
{
    fprintf(stderr, "usage: NetPing udp|tcp <hostname> <port>\n");
    exit(1);
}

void udp_ping(char * hostname, int port)
{
    UdpSocket sender;
    YETI_Result result;

    DataBuffer packet;
    const char * packet_data = "PING";
    packet.set_data((YETI_Byte *)packet_data, sizeof(packet_data));

    IpAddress ip_address;
    result = ip_address.resolve_name(hostname);

    if (YETI_FAILED(result)) {
        yeti_debug("ERROR: failed to resolve name\n");
        return;
    }

    TimeStamp before;
    System::get_current_timestamp(before);

    sender.connect(SocketAddress(ip_address, port));
    sender.send(packet);
    SocketInfo socket_info;
    sender.get_info(socket_info);
    yeti_debug("sent from %s:%d to %s:%d\n",
        socket_info.local_address.get_ipaddress().to_string().get_chars(),
        socket_info.local_address.get_port(),
        socket_info.remote_address.get_ipaddress().to_string().get_chars(),
        socket_info.remote_address.get_port());
    SocketAddress destination_address(ip_address, port);
    sender.send(packet, &destination_address);
    yeti_debug("send %d bytes\n", 4);

    DataBuffer response(32768);
    result = sender.receive(response);

    if (YETI_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to receive response (%d)\n", result);
        return;
    }

    sender.get_info(socket_info);
    printf("RESPONSE: %d bytes from %s:%d\n", response.get_data_size(),
        socket_info.remote_address.get_ipaddress().to_string().get_chars(),
        socket_info.remote_address.get_port());
    TimeStamp after;
    System::get_current_timestamp(after);
    TimeInterval i = after - before;
    printf("RTT: %f ms\n", ((float)i) * 1000.0f);
}

void tcp_ping(char * hostname, int port)
{
    TcpClientSocket sender;
    YETI_Result result;

    IpAddress ip_address;
    result = ip_address.resolve_name(hostname);
    if (YETI_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to resolve name\n");
        return;
    }

    sender.connect(SocketAddress(ip_address, port));
    OutputStreamReference output;
    TimeStamp before;
    System::get_current_timestamp(before);
    sender.get_output_stream(output);
    output->write("PING", 4);
    InputStreamReference input;
    sender.get_input_stream(input);
    char buffer[1024];
    YETI_Size bytes_read;
    result = input->read(buffer, sizeof(buffer), &bytes_read);
    if (YETI_SUCCEEDED(result)) {
        yeti_debug("read %ld bytes\n", bytes_read);
    }
    TimeStamp after;
    System::get_current_timestamp(after);
    TimeInterval i = after - before;
    printf("RTT: %f ms\n", ((float)i) * 1000.0f);
}

int ping_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    if (args.size() != 5) {
        ping_print_useage_and_exit();
    }

    Options.verbose = true;
    ClientType client_type = CLIENT_TYPE_UNKNOWN;
    int port = -1;
    char * hostname = NULL;

    if (args[2] == "udp") {
        client_type = CLIENT_TYPE_UDP;
    } else if (args[2] == "tcp") {
        client_type = CLIENT_TYPE_TCP;
    } else {
        fprintf(stderr, "ERROR: unknown client type\n");
        exit(1);
    }

    hostname = (char *)args[3].c_str();
    port = strtoul(args[4].c_str(), NULL, 10);

    switch (client_type) {
case CLIENT_TYPE_TCP: tcp_ping(hostname, port); break;
case CLIENT_TYPE_UDP: udp_ping(hostname, port); break;
default:break;
    }

    return 0;
}

static TestRegister test("ping_test", ping_test);
