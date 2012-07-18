#include "Common.h"

USINGNAMESPACE2;

typedef enum {
    SERVER_TYPE_UNKNOWN,
    SERVER_TYPE_UDP,
    SERVER_TYPE_TCP
} ServerType;

static struct {
    bool verbose;
} Options;

void echo_print_useage_and_exit()
{
    fprintf(stderr, "usage: NetEcho udp|tcp <port>\n");
    exit(1);
}

void udp_server_loop(int port)
{
    UdpSocket listener;
    if (Options.verbose) {
        yeti_debug("listening on port %d\n", port);
    }

    YETI_Result result = listener.bind(SocketAddress(IpAddress::Any, port));
    if (YETI_FAILED(result)) {
        yeti_debug("ERROR: bind() failed (%d : %s)\n", result, error_message(result));
    }

    DataBuffer packet(32768);
    SocketAddress address;

    do {
        result = listener.receive(packet, &address);
        if (YETI_SUCCEEDED(result)) {
            if (Options.verbose) {
                String ip = address.get_ipaddress().to_string();
                yeti_debug("Received %d bytes from %s:%d\n", packet.get_data_size(), ip.get_chars(), address.get_port());
            }
            listener.send(packet, &address);
        }
    } while (YETI_SUCCEEDED(result));
}

void tcp_server_loop(int port)
{
    TcpServerSocket listener;

    YETI_Result result = listener.bind(SocketAddress(IpAddress::Any, port));
    if (YETI_FAILED(result)) {
        yeti_debug("ERROR: bind() failed (%d : %s)\n", result, error_message(result));
    }

    Socket * client;

    for (;;) {
        yeti_debug("waiting for client on port %d\n", port);
        result = listener.wait_for_new_client(client);
        SocketInfo socket_info;
        client->get_info(socket_info);
        yeti_debug("client connected from %s : %d\n", socket_info.remote_address.get_ipaddress().to_string().get_chars(), socket_info.remote_address.get_port());
        InputStreamReference input;
        client->get_input_stream(input);
        OutputStreamReference output;
        client->get_output_stream(output);
        do {
            char buffer[1024];
            YETI_Size bytes_read;
            result = input->read(buffer, sizeof(buffer), &bytes_read);
            if (YETI_SUCCEEDED(result)) {
                yeti_debug("read %ld bytes\n", bytes_read);
                output->write(buffer, bytes_read);
            }
        } while(YETI_SUCCEEDED(result));
        delete client;
    }
}

int echo_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    if (args.size() != 4) {
        echo_print_useage_and_exit();
    }

    Options.verbose = true;
    ServerType server_type = SERVER_TYPE_UNKNOWN;
    int port = -1;

    if (args[2] == "udp") {
        server_type = SERVER_TYPE_UDP;
    } else if (args[2] == "tcp") {
        server_type = SERVER_TYPE_TCP;
    } else {
        fprintf(stderr, "ERROR: unknown server type\n");
        exit(1);
    }

    port = strtoul(args[3].c_str(), NULL, 10);

    switch (server_type) {
case SERVER_TYPE_TCP: tcp_server_loop(port); break;
case SERVER_TYPE_UDP: udp_server_loop(port); break;
default:break;
    }

    return 0;
}

static TestRegister test("echo_test", echo_test);
