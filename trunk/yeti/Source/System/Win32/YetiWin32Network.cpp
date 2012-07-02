#define STRICT

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "YetiNetwork.h"
#include "YetiWin32Network.h"

NAMEBEG

WinsockSystem::WinsockSystem()
{
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(2, 2);
    ::WSAStartup(wVersionRequested, &wsaData);
}

WinsockSystem::~WinsockSystem()
{
    ::WSACleanup();
}

WinsockSystem WinsockSystem::Initializer;

#if defined(_WIN32_WCE)
#define YETI_NETWORK_USE_IP_HELPER_API
#else
#define  YETI_NETWORK_USE_SIO_GET_INTERFACE_LIST
#endif

#if defined(YETI_NETWORK_USE_SIO_GET_INTERFACE_LIST)
YETI_Result NetworkInterface::get_network_interfaces(List<NetworkInterface *> & interfaces)
{
    SOCKET net;
    if ((net = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0)) == INVALID_SOCKET) {
        return YETI_FAILURE;
    }

    INTERFACE_INFO query[32];
    DWORD bytes_returned;
    int io_result = WSAIoctl(net,
        SIO_GET_INTERFACE_LIST,
        NULL, 0,
        &query, sizeof(query),
        &bytes_returned,
        NULL, NULL);
    if (io_result == SOCKET_ERROR) {
        closesocket(net);
        return YETI_FAILURE;
    }

    closesocket(net);

    int interface_count = (bytes_returned / sizeof(INTERFACE_INFO));
    unsigned int iface_index = 0;
    for (int i = 0; i < interface_count; ++i) {
        SOCKADDR_IN * address;
        YETI_Flags flags = 0;
        // primary address
        address = (SOCKADDR_IN *)&query[i].iiAddress;
        IpAddress primary_address(ntohl(address->sin_addr.s_addr));
        // netmask address
        address = (SOCKADDR_IN *)&query[i].iiNetmask;
        IpAddress netmask(ntohl(address->sin_addr.s_addr));
        // broadcast address
        address = (SOCKADDR_IN *)&query[i].iiBroadcastAddress;
        IpAddress broadcast_address(ntohl(address->sin_addr.s_addr));

        {
            // broadcast address is incorrect
            unsigned char addr[4];
            for (int i = 0; i < 4; ++i) {
                addr[i] = (primary_address.as_bytes()[i] & netmask.as_bytes()[i] | ~netmask.as_bytes()[i]);
            }

            broadcast_address.set(addr);
        }

        // ignore interfaces that are not up
        if (!(query[i].iiFlags & IFF_UP)) {
            continue;
        }
        if (query[i].iiFlags & IFF_BROADCAST) {
            flags |= YETI_NETWORK_INTERFACE_FLAG_BROADCAST;
        }
        if (query[i].iiFlags & IFF_MULTICAST) {
            flags |= YETI_NETWORK_INTERFACE_FLAG_MULTICAST;
        }
        if (query[i].iiFlags & IFF_LOOPBACK) {
            flags |= YETI_NETWORK_INTERFACE_FLAG_LOOPBACK;
        }
        if (query[i].iiFlags & IFF_POINTTOPOINT) {
            flags |= YETI_NETWORK_INTERFACE_FLAG_POINT_TO_POINT;
        }

        // mac address (no support for this for now)
        MacAddress mac;

        // create an interface object
        char iface_name[5];
        iface_name[0] = 'i';
        iface_name[1] = 'f';
        iface_name[2] = '0' + (iface_index / 10);
        iface_name[3] = '0' + (iface_index % 10);
        iface_name[4] = '\0';

        NetworkInterface * iface = new NetworkInterface(iface_name, mac, flags);
        // set the interface address
        NetworkInterfaceAddress iface_address(primary_address,
            broadcast_address,
            IpAddress::Any,
            netmask);
        iface->add_address(iface_address);
        // add the interface to the list
        interfaces.add(iface);
        // increment the index (used for generating the name
        iface_index++;
    }
    return YETI_SUCCESS;
}
#elif defined(YETI_NETWORK_USE_IP_HELPER_API)
#include <iphlpapi.h>
YETI_Result NetworkInterface::get_network_interfaces(List<NetworkInterface *> & interfaces)
{
    IP_ADAPTER_ADDRESSES * iface_list = NULL;
    ULONG                 size = sizeof(IP_ADAPTER_INFO);

    // get the interface table
    for(;;) {
        iface_list = (IP_ADAPTER_ADDRESSES *)malloc(size);
        DWORD result = GetAdaptersAddresses(AF_INET,
            0,
            NULL,
            iface_list, &size);
        if (result == NO_ERROR) {
            break;
        } else {
            if (result == ERROR_BUFFER_OVERFLOW) {
                // free and try again
                free(iface_list);
            } else {
                return YETI_FAILURE;
            }
        }
    }

    // iterate over the interfaces
    for (IP_ADAPTER_ADDRESSES* iface = iface_list; iface; iface = iface->Next) {
        // skip this interface if it is not up
        if (iface->OperStatus != IfOperStatusUp) continue;

        // get the interface type and mac address
        MacAddress::Type mac_type;
        switch (iface->IfType) {
            case IF_TYPE_ETHERNET_CSMACD:   mac_type = MacAddress::TYPE_ETHERNET; break;
            case IF_TYPE_SOFTWARE_LOOPBACK: mac_type = MacAddress::TYPE_LOOPBACK; break;
            case IF_TYPE_PPP:               mac_type = MacAddress::TYPE_PPP;      break;
            default:                        mac_type = MacAddress::TYPE_UNKNOWN;  break;
        }
        MacAddress mac(mac_type, iface->PhysicalAddress, iface->PhysicalAddressLength);

        // compute interface flags
        YETI_Flags flags = 0;
        if (!(iface->Flags & IP_ADAPTER_NO_MULTICAST)) flags |= YETI_NETWORK_INTERFACE_FLAG_MULTICAST;
        if (iface->IfType == IF_TYPE_SOFTWARE_LOOPBACK) flags |= YETI_NETWORK_INTERFACE_FLAG_LOOPBACK;
        if (iface->IfType == IF_TYPE_PPP) flags |= YETI_NETWORK_INTERFACE_FLAG_POINT_TO_POINT;

        // compute the unicast address (only the first one is supported for now)
        IpAddress primary_address;
        if (iface->FirstUnicastAddress) {
            if (iface->FirstUnicastAddress->Address.lpSockaddr == NULL) continue;
            if (iface->FirstUnicastAddress->Address.iSockaddrLength != sizeof(SOCKADDR_IN)) continue;
            SOCKADDR_IN* address = (SOCKADDR_IN*)iface->FirstUnicastAddress->Address.lpSockaddr;
            if (address->sin_family != AF_INET) continue;
            primary_address.Set(ntohl(address->sin_addr.s_addr));
        }
        IpAddress broadcast_address; // not supported yet
        IpAddress netmask;           // not supported yet

        // convert the interface name to UTF-8
        // BUG in Wine: FriendlyName is NULL
        unsigned int iface_name_length = (unsigned int)iface->FriendlyName ? wcslen(iface->FriendlyName): 0;
        char* iface_name = new char[4*iface_name_length+1];
        int result = WideCharToMultiByte(
            CP_UTF8, 0, iface->FriendlyName, iface_name_length,
            iface_name, 4*iface_name_length+1,
            NULL, NULL);
        if (result > 0) {
            iface_name[result] = '\0';
        } else {
            iface_name[0] = '\0';
        }

        // create an interface descriptor
        NetworkInterface* iface_object = new NetworkInterface(iface_name, mac, flags);
        NetworkInterfaceAddress iface_address(
            primary_address,
            broadcast_address,
            IpAddress::Any,
            netmask);
        iface_object->AddAddress(iface_address);  

        // cleanup 
        delete[] iface_name;

        // add the interface to the list
        interfaces.Add(iface_object);   
    }
    return YETI_SUCCESS;
}
#else
YETI_Result NetworkInterface::get_network_interfaces(List<NetworkInterface *> & interfaces)
{
    return YETI_SUCCESS;
}
#endif

NAMEEND
