#ifndef _ILIBDNSSD_H_
#define _ILIBDNSSD_H_

#include "ILibParsers.h"

#if defined( ENABLED_AIRPLAY )

int ILibDaemonIsRunning();
void * ILibCreateDnssdModule(void * chain,
                             char * fcr_type,
                             char * fcr_name,
                             unsigned short f_port,
                             void * txt_map,
                             void (* OnDnssdStart)(int error_code, void * user),
                             void * user);
int ILibDnssdIsRunning(void * dnssd_token);

#endif // ENABLED_AIRPLAY
#endif // _ILIBDNSSD_H_
