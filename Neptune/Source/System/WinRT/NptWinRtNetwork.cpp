/*****************************************************************
|
|      Neptune - Network :: WinRT Implementation
|
|      (c) 2011-2012 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptWinRtPch.h"

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptStreams.h"
#include "NptThreads.h"
#include "NptNetwork.h"
#include "NptUtils.h"
#include "NptConstants.h"
#include "NptSockets.h"

using namespace Platform;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Foundation::Collections;
/*----------------------------------------------------------------------
|   NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces)
{// ��δ���
   IVectorView<HostName^>^ hostnames = NetworkInformation::GetHostNames();
   NPT_NetworkInterface * iface;
   for (unsigned int i = 0; i < hostnames->Size; ++i) {
       Platform::String^ dbg_str = L"";
       HostName^ hostname = hostnames->GetAt(i);
       dbg_str += L"RawName = ";
       dbg_str += hostname->RawName;
       dbg_str += L" CanonicalName = ";
       dbg_str += hostname->CanonicalName;
       dbg_str += L" DisplayName = ";
       dbg_str += hostname->DisplayName;
       dbg_str += L" Type = ";
       switch (hostname->Type) {
       case HostNameType::Bluetooth:
           dbg_str += "Bluetooth";
           break;
       case HostNameType::DomainName:
           dbg_str += "DomainName";
           break;
       case HostNameType::Ipv4:
       case HostNameType::Ipv6:
           {
               if (hostname->Type == HostNameType::Ipv4) {
                   dbg_str += "Ipv4";
               } else {
                   dbg_str += "Ipv6";
               }
           }
           {
               char iface_name[5];
               iface_name[0] = 'i';
               iface_name[1] = 'f';
               iface_name[2] = '0'+(i/10);
               iface_name[3] = '0'+(i%10);
               iface_name[4] = '\0';
               iface = new NPT_NetworkInterface(iface_name, NPT_NETWORK_INTERFACE_FLAG_MULTICAST);
               NPT_IpAddress primary_address;
               primary_address.Parse(Str2NPT_Str(hostname->RawName).GetChars());
               NPT_NetworkInterfaceAddress iface_address(
                   primary_address,
                   NPT_IpAddress::Any,
                   NPT_IpAddress::Any,
                   NPT_IpAddress::Any);
               iface->AddAddress(iface_address);
               interfaces.Add(iface);
           }
           break;
       default:
           dbg_str += "Unknown Type";
       }

       OutputDebugStringW(dbg_str->Data());
       OutputDebugStringW(L"\r\n");
   }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::ResolveName
+---------------------------------------------------------------------*/
NPT_Result
NPT_IpAddress::ResolveName(const char* name, NPT_Timeout timeout)
{
	m_HostName = name;
	return NPT_SUCCESS;
}