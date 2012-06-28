#ifndef _CXL_YETI_NETWORK_H_
#define _CXL_YETI_NETWORK_H_

#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiString.h"
#include "YetiList.h"

NAMEBEG

const unsigned int YETI_NETWORK_MAX_MAC_ADDRESS_LENGTH = 8;

#define YETI_NETWORK_INTERFACE_FLAG_LOOPBACK         0x01
#define YETI_NETWORK_INTERFACE_FLAG_PROMISCUOUS      0x02
#define YETI_NETWORK_INTERFACE_FLAG_BROADCAST        0x04
#define YETI_NETWORK_INTERFACE_FLAG_MULTICAST        0x08
#define YETI_NETWORK_INTERFACE_FLAG_POINT_TO_POINT   0x10

#if defined(_WIN32)
#   if defined(SetPort)
#       undef  SetPort
#   endif
#endif

typedef unsigned int IpPort;

class IpAddress
{
public:
    static const IpAddress Any;

    IpAddress();
    IpAddress(unsigned long address);
    IpAddress(unsigned char a, unsigned char b, unsigned char c, unsigned char d);

    YETI_Result resolve_name(const char * name, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE);
    YETI_Result parse(const char * name);
    YETI_Result set(unsigned long address);
    YETI_Result set(const unsigned char bytes[4]);
    const unsigned char * as_bytes() const;
    unsigned long as_long() const;
    String to_string() const;

    bool operator==(const IpAddress & other) const;

private:
    unsigned char m_address_[4];
};

class MacAddress
{
public:
    typedef enum {
        TYPE_UNKNOWN,
        TYPE_LOOKBACK,
        TYPE_ETHERNET,
        TYPE_PPP,
        TYPE_IEEE_802_11
    } Type;

    MacAddress() : m_type_(TYPE_UNKNOWN), m_length_(0) {}
    MacAddress(Type type,
        const unsigned char * addr,
        unsigned int length);

    void set_address(Type type, const unsigned char * addr, unsigned int length);
    Type get_type() const { return m_type_; }
    const unsigned char * get_address() const { return m_address_; }
    unsigned int get_length() const { return m_length_; }
    String to_string() const;
private:
    Type m_type_;
    unsigned char m_address_[YETI_NETWORK_MAX_MAC_ADDRESS_LENGTH];
    unsigned int m_length_;
};

class NetworkInterfaceAddress
{
public:
    NetworkInterfaceAddress(const IpAddress & primary,
        const IpAddress & broadcast,
        const IpAddress & destination,
        const IpAddress & netmask)
        : m_primary_address_(primary)
        , m_broadcast_address_(broadcast)
        , m_destination_address_(destination)
        , m_netmask_(netmask)
    {
    }

    const IpAddress & get_primary_address() const {
        return m_primary_address_;
    }

    const IpAddress & get_broadcast_address() const {
        return m_broadcast_address_;
    }

    const IpAddress & get_destination_address() const {
        return m_destination_address_;
    }

    const IpAddress & get_netmask() const {
        return m_netmask_;
    }

    bool is_address_in_network(const IpAddress & address) {
        if (m_primary_address_.as_long() == address.as_long()) return true;
        if (m_netmask_.as_long() == 0) return false;
        return ((m_primary_address_.as_long() & m_netmask_.as_long()) == (address.as_long() & m_netmask_.as_long()));
    }
private:
    IpAddress m_primary_address_;
    IpAddress m_broadcast_address_;
    IpAddress m_destination_address_;
    IpAddress m_netmask_;

};

class NetworkInterface
{
public:
    static YETI_Result get_network_interfaces(List<NetworkInterface *> & interfaces);

    NetworkInterface(const char * name,
        const MacAddress & mac,
        YETI_Flags flags);
    NetworkInterface(const char * name,
        YETI_Flags flags);
    ~NetworkInterface() {}

    YETI_Result add_address(const NetworkInterfaceAddress & address);
    const String & get_name() const {
        return m_name_;
    }
    const MacAddress & get_mac_address() const {
        return m_mac_address_;
    }
    void set_mac_address(MacAddress::Type type,
        const unsigned char * addr,
        unsigned int length) {
            m_mac_address_.set_address(type, addr, length);
    }
    YETI_Flags get_flags() const { return m_flags_; }
    const List<NetworkInterfaceAddress> & get_addresses() const {
        return m_addresses_;
    }

    bool is_address_in_network(const IpAddress & address) {
        List<NetworkInterfaceAddress>::iterator iter = m_addresses_.get_first_item();
        while (iter) {
            if ((*iter).is_address_in_network(address)) return true;
            ++iter;
        }
        return false;
    }
private:
    String m_name_;
    MacAddress m_mac_address_;
    YETI_Flags m_flags_;
    List<NetworkInterfaceAddress> m_addresses_;
};

// ”Ú√˚Ω‚Œˆ
class NetworkNameResolver
{
public:
    static YETI_Result resolve(const char * name,
        List<IpAddress> & addresses,
        YETI_Timeout timeout = YETI_TIMEOUT_INFINITE);
};

NAMEEND

#endif // _CXL_YETI_NETWORK_H_
