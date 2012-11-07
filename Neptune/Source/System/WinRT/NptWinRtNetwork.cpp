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
{// ÉÐÎ´Íê³É
   IVectorView<HostName^>^ hostnames = NetworkInformation::GetHostNames();
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
           {
               dbg_str += "Ipv4";
               NPT_NetworkInterface * ifs = new NPT_NetworkInterface(Str2NPT_Str(hostname->RawName), NPT_NETWORK_INTERFACE_FLAG_MULTICAST);
               interfaces.Add(ifs);
           }
           break;
       case HostNameType::Ipv6:
           dbg_str += "Ipv6";
           break;
       default:
           dbg_str += "Unknown Type";
       }

       OutputDebugStringW(dbg_str->Data());
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