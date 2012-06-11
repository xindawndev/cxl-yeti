#include "Common.h"
//#include <windows.h>

USINGNAMESPACE2;

int test_socket()
{
    //int PortNumber = 0;
    //int count               = 0;
    //int ra                  = 1;
    //unsigned short PortNum  = -1;
    //struct sockaddr_in addr, remote_addr;
    //memset((char *)&(addr), 0, sizeof(addr));
    //addr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = 0;

    //memset((char *)&remote_addr, 0,sizeof(remote_addr));
    //remote_addr.sin_family = AF_INET;
    //remote_addr.sin_addr.s_addr = inet_addr("192.168.1.128");;
    //remote_addr.sin_port = htons((unsigned short)46115);


    //WORD wVersionRequested;
    //WSADATA wsaData;
    //wVersionRequested = MAKEWORD( 2, 0 );
    //if (WSAStartup( wVersionRequested, &wsaData ) != 0) {exit(1);}

    //SOCKET TheSocket;

    //TheSocket = socket(AF_INET, SOCK_STREAM, 0);
    //if(PortNumber == 0)
    //{
    //    do
    //    {
    //        if (++count >= 20)
    //        {
    //            PortNum=0;
    //            break;
    //        }
    //        PortNum = (unsigned short)(40000 + ((unsigned short)rand() % 15000));
    //        addr.sin_port = htons(PortNum);
    //    }
    //    while(bind(TheSocket, (struct sockaddr *) &(addr), sizeof(addr)) < 0);
    //}
    //else
    //{
    //    addr.sin_port = htons(PortNumber);
    //    if (setsockopt(TheSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&ra, sizeof(ra)) < 0)
    //    {
    //    }
    //    PortNum = bind(TheSocket, (struct sockaddr *) &(addr), sizeof(addr)) < 0 ? 0: PortNumber;
    //}

    //int flags = 1;
    ////ioctlsocket(TheSocket,FIONBIO,(u_long *)&flags);
    //flags = 1;
    //setsockopt(TheSocket,SOL_SOCKET,SO_KEEPALIVE,(char*)&flags,sizeof(flags));

    //long beg = GetTickCount();
    //int ret = connect(TheSocket,(struct sockaddr*)&remote_addr,sizeof(remote_addr));

    //printf("ErrorCode = %d, %d, timeout = %ldms\n", ret, GetLastError(), GetTickCount() - beg);

    return 0;
}


int socket_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_socket();

    return 0;
}

static TestRegister test("socket_test", socket_test);
