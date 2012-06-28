#include "YetiNetwork.h"
#include "YetiUtil.h"

NAMEBEG

const IpAddress IpAddress::Any;

IpAddress::IpAddress()
{
    m_address_[0] = m_address_[1] = m_address_[2] = m_address_[3] = 0;
}

IpAddress::IpAddress(unsigned long address)
{
    set(address);
}

IpAddress::IpAddress(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
    m_address_[0] = a;
    m_address_[1] = b;
    m_address_[2] = c;
    m_address_[3] = d;
}

YETI_Result IpAddress::parse(const char * name)
{
    if (name == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    m_address_[0] = m_address_[1] = m_address_[2] = m_address_[3] = 0;
    unsigned int fragment;
    bool fragment_empty = true;
    unsigned char address[4];
    unsigned int accumulator;
    for (fragment = 0, accumulator = 0; fragment < 4; ++name) {
        if (*name == '\0' || *name == '.') {
            if (fragment_empty) return YETI_ERROR_INVALID_SYNTAX;
            address[fragment++] = accumulator;
            if (*name == '\0') break;
            accumulator = 0;
            fragment_empty = true;
        } else if (*name >= '0' && *name <= '9') {
            accumulator = accumulator * 10 + (*name - '0');
            if (accumulator > 255) return YETI_ERROR_INVALID_SYNTAX;
            fragment_empty = false;
        } else {
            return YETI_ERROR_INVALID_SYNTAX;
        }
    }

    if (fragment == 4 && *name == '0' && !fragment_empty) {
        m_address_[0] = address[0];
        m_address_[1] = address[1];
        m_address_[2] = address[2];
        m_address_[3] = address[3];
        return YETI_SUCCESS;
    }

    return YETI_ERROR_INVALID_SYNTAX;
}

unsigned long IpAddress::as_long() const
{
    return 
        ((((unsigned long)m_address_[0]) << 24) ||
        (((unsigned long)m_address_[1]) << 16) ||
        (((unsigned long)m_address_[2]) << 8) ||
        (((unsigned long)m_address_[3])));
}

const unsigned char * IpAddress::as_bytes() const
{
    return m_address_;
}

String IpAddress::to_string() const
{
    String address;
    address.reserve(16);
    address += String::from_integer(m_address_[0]);
    address += '.';
    address += String::from_integer(m_address_[1]);
    address += '.';
    address += String::from_integer(m_address_[2]);
    address += '.';
    address += String::from_integer(m_address_[3]);
    address += '.';
    
    return address;
}

YETI_Result IpAddress::set(const unsigned char bytes[4])
{
    m_address_[0] = bytes[0];
    m_address_[1] = bytes[1];
    m_address_[2] = bytes[2];
    m_address_[3] = bytes[3];

    return YETI_SUCCESS;
}

YETI_Result IpAddress::set(unsigned long address)
{
    m_address_[0] = (unsigned char)((address >> 24) & 0xFF);
    m_address_[1] = (unsigned char)((address >> 16) & 0xFF);
    m_address_[2] = (unsigned char)((address >>  8) & 0xFF);
    m_address_[3] = (unsigned char)((address      ) & 0xFF);

    return YETI_SUCCESS;
}

bool IpAddress::operator ==(const IpAddress & other) const
{
    return other.as_long() == as_long();
}

MacAddress::MacAddress(Type type,
                       const unsigned char * address,
                       unsigned int length)
{
    set_address(type, address, length);
}

void MacAddress::set_address(Type type,
                             const unsigned char * address,
                             unsigned int length)
{
    m_type_ = type;
    if (length > YETI_NETWORK_MAX_MAC_ADDRESS_LENGTH) {
        length = YETI_NETWORK_MAX_MAC_ADDRESS_LENGTH;
    }
    m_length_ = length;
    for (unsigned int i = 0; i < length; ++i) {
        m_address_[i] = address[i];
    }
}

String MacAddress::to_string() const
{
    String result;
    if (m_length_) {
        char s[3 * YETI_NETWORK_MAX_MAC_ADDRESS_LENGTH];
        const char hex[17] = "0123456789abcdef";
        for (unsigned int i = 0; i < m_length_; ++i) {
            s[i * 3    ] = hex[m_address_[i] >> 4];
            s[i * 3 + 1] = hex[m_address_[i] & 0xf];
            s[i * 3 + 2] = ':';
        }
        s[3 * m_length_ - 1] = '\0';
        result = s;
    }

    return result;
}

NetworkInterface::NetworkInterface(const char * name,
                                   const MacAddress & mac,
                                   YETI_Flags flags)
                                   : m_name_(name)
                                   , m_mac_address_(mac)
                                   , m_flags_(flags)
{
}

YETI_Result NetworkInterface::add_address(const NetworkInterfaceAddress & address)
{
    return m_addresses_.add(address);
}

NAMEEND
