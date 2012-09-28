// ILibDnssd.c
#include "ILibParsers.h"

#if defined( ENABLED_AIRPLAY )

#ifdef WIN32
#   include "dns_sd.h"
#else//lif defined(__ANDROID__)
#   include "mDNSEmbeddedAPI.h"
#   include "mDNSPosix.h"
#   include "mDNSUNP.h"
const char ProgramName[] = "ILibDnssd";
#endif

#include "ILibDnssd.h"

#define CreateString(x)        (char *)strcpy((char *)malloc(strlen(x) + 1), x)

#ifdef WIN32

struct DnssdObject
{
    ILibChain_PreSelect     pre_select;
    ILibChain_PostSelect    post_select;
    ILibChain_Destroy       destroy;

    void (* OnDnssdStartCallback)(int error_code, void * user);
    char *                  fcr_type;
    char *                  fcr_name;
    unsigned short          f_port;
    void *                  txt_map;
    DNSServiceRef           net_service;
    void *                  user;
    int                     terminate;
};

void DNSSD_API ILibDnssdRegisterCallback(DNSServiceRef sdref, const DNSServiceFlags flags, DNSServiceErrorType error_code, const char *name, const char *regtype, const char *domain, void *context)
{
    struct DnssdObject * object = (struct DnssdObject *)context;
    (void)sdref;
    (void)flags;

    if (error_code == kDNSServiceErr_NoError) {
        if (flags & kDNSServiceFlagsAdd) {
            printf("Zeroconf: %s.%s%s now registered and active\n", name, regtype, domain);
        } else {
            printf("Zeroconf: %s.%s%s registration removed\n", name, regtype, domain);
        }
    } else if (error_code == kDNSServiceErr_NameConflict) {
        printf("Zeroconf: %s.%s%s Name in use, please choose another\n", name, regtype, domain);
    } else {
        printf("Zeroconf: %s.%s%s error code %d\n", name, regtype, domain, error_code);
    }
    if (object != NULL && object->OnDnssdStartCallback != NULL) {
        object->OnDnssdStartCallback(error_code, object->user);
    }
}

void ILibDnssdStart(struct DnssdObject * object)
{
    void * iter             = NULL;
    char * key              = NULL;
    int  len                = 0;
    char * val              = NULL;
    DNSServiceErrorType ec;
    TXTRecordRef txt_record;
    TXTRecordCreate(&txt_record, 0, NULL);

    printf("Start Zeroconf: type: %s name:%s port:%i\n", object->fcr_type, object->fcr_name, object->f_port);

    iter = ILibHashTree_GetEnumerator(object->txt_map);
    while ( !ILibHashTree_MoveNext( iter ) ) {
        ILibHashTree_GetValue( iter, &key, &len, ((void **)(&val)));
        TXTRecordSetValue(&txt_record, key, strlen(val), val);
        printf("Zeroconf: key:%s, value:%s\n",key,val);
    }
    ILibHashTree_DestroyEnumerator(iter);

    ec = DNSServiceRegister(&object->net_service, 0, 0, object->fcr_name, object->fcr_type, NULL, NULL, htons(object->f_port), TXTRecordGetLength(&txt_record), TXTRecordGetBytesPtr(&txt_record), &ILibDnssdRegisterCallback, (void *)object);

    if (ec != kDNSServiceErr_NoError) {
        if (object->net_service) {
            DNSServiceRefDeallocate(object->net_service);
        }
        printf("Zeroconf: DNSServiceRegister returned (error = %ld)\n", (int)ec);
    } else {
        ec = DNSServiceProcessResult(object->net_service);

        if (ec != kDNSServiceErr_NoError) {
            printf("Zeroconf: DNSServiceProcessResult returned (error = %ld)\n", (int)ec);
        }
    }

    TXTRecordDeallocate(&txt_record);
}

void ILibDnssdPreSelect(void * object,void * readset, void * writeset, void * errorset, int * blocktime)
{
    struct DnssdObject *s   = (struct DnssdObject *)object;
    ILibDnssdStart(s);
    s->terminate            = 0;
    s->pre_select = NULL;
}

int ILibStopDnssdModule(void * dnssd_token)
{
    struct DnssdObject * s  = (struct DnssdObject *)dnssd_token;
    if (s != NULL && s->net_service != NULL) {
        DNSServiceRefDeallocate(s->net_service);
        return 1;
    }
    return 0;
}

void ILibDnssdDestroy(void * object)
{
    void * iter             = NULL;
    char * key              = NULL;
    int  len                = 0;
    char * val              = NULL;
    struct DnssdObject * s  = (struct DnssdObject *)object;

    freesafe(s->fcr_name);
    freesafe(s->fcr_type);

    ILibStopDnssdModule(object);

    iter = ILibHashTree_GetEnumerator(s->txt_map);
    while ( !ILibHashTree_MoveNext( iter ) ) {
        ILibHashTree_GetValue( iter, &key, &len, ((void **)(&val)));
        freesafe(val);
    }
    ILibHashTree_DestroyEnumerator(iter);
    ILibDestroyHashTree(s->txt_map);
}

int ILibDaemonIsRunning()
{
    int version;
    int size = sizeof(version);
    DNSServiceErrorType ec = DNSServiceGetProperty(kDNSServiceProperty_DaemonVersion, &version, &size);
    if (ec != kDNSServiceErr_NoError) {
        printf("Zeroconf: Zeroconf can't be started probably because Apple's Bonjour Service isn't installed. You can get it by either installing Itunes or Apple's Bonjour Print Service for Windows (http://support.apple.com/kb/DL999)\n");
        return 0;
    }
    printf("Zeroconf:Bonjour version is %d.%d\n", version / 10000, version / 100 % 100);
    return 1;
}

int ILibDnssdIsRunning(void * dnssd_token)
{
    struct DnssdObject * s  = (struct DnssdObject *)dnssd_token;
    if (s == NULL) return 0;
    return (s->terminate == 0 ? 1: 0);
}


void * ILibCreateDnssdModule(void * chain,
                             char * fcr_type,
                             char * fcr_name,
                             unsigned short f_port,
                             void * txt_map,
                             void (* OnDnssdStart)(int error_code, void * user),
                             void * user)
{
    struct DnssdObject * ret_val    = (struct DnssdObject *)malloc(sizeof(struct DnssdObject));
    void * iter                     = NULL;
    char * key                      = NULL;
    int  len                        = 0;
    char * val                      = NULL;
    memset(ret_val, 0, sizeof(struct DnssdObject));
    ret_val->pre_select             = &ILibDnssdPreSelect;
    ret_val->post_select            = NULL;
    ret_val->destroy                = &ILibDnssdDestroy;
    ret_val->fcr_name               = CreateString(fcr_name);
    ret_val->fcr_type               = CreateString(fcr_type);
    ret_val->f_port                 = f_port;
    ret_val->terminate              = 1;
    ret_val->OnDnssdStartCallback   = OnDnssdStart;
    ret_val->net_service            = NULL;

    ret_val->txt_map                = ILibInitHashTree();

    iter = ILibHashTree_GetEnumerator(txt_map);
    while ( !ILibHashTree_MoveNext( iter ) ) { // 注意：多一次指针引用，使用完毕由内部释放，外部创建不用释放
        ILibHashTree_GetValue( iter, &key, &len, ((void **)(&val)));
        ILibAddEntry(ret_val->txt_map, key, len, ( void * )val );
    }
    ILibHashTree_DestroyEnumerator(iter);

    ILibAddToChain(chain, (void *)ret_val);

    return (void *)ret_val;
}

#else //lif defined(__ANDROID__)

struct DnssdObject
{
    ILibChain_PreSelect     pre_select;
    ILibChain_PostSelect    post_select;
    ILibChain_Destroy       destroy;

    void (* OnDnssdStartCallback)(int error_code, void * user);
    char *                  fcr_type;
    char *                  fcr_name;
    char *                  service_domain;
    unsigned short          f_port;
    mDNSu8                  service_text[sizeof(RDataBody)];
    mDNSu16                 service_text_len;
    void *                  process_timer;
    void *                  user;
    mDNS                    dns_storage;
    mDNS_PlatformSupport    platform_storage;
    ServiceRecordSet        core_serv;
    int                     terminate;
};

void ILibDnssdLoop(struct DnssdObject * object)
{
    int nfds = 0;
    fd_set readfds;
    struct timeval timeout;
    int result;

    FD_ZERO(&readfds);

    timeout.tv_sec = 0x3FFFFFFF;
    timeout.tv_usec = 0;

    mDNSPosixGetFDSet(&object->dns_storage, &nfds, &readfds, &timeout);

    verbosedebugf("select(%d, %d.%06d)", nfds, timeout.tv_sec, timeout.tv_usec);
    result = select(nfds, &readfds, NULL, NULL, &timeout);

    if (result < 0)
    {
        verbosedebugf("select() returned %d errno %d", result, errno);
        printf("select() returned %d errno %d", result, errno);
        ILibLifeTime_Remove(object->process_timer, NULL);
        return;
        if (errno != EINTR) {
            ILibLifeTime_Remove(object->process_timer, NULL);
            return;
        } else {
            //if (gReceivedSigUsr1)
            //{
            //    gReceivedSigUsr1 = mDNSfalse;
            //    gMDNSPlatformPosixVerboseLevel += 1;
            //    if (gMDNSPlatformPosixVerboseLevel > 2)
            //        gMDNSPlatformPosixVerboseLevel = 0;
            //    if ( gMDNSPlatformPosixVerboseLevel > 0 )
            //        fprintf(stderr, "\nVerbose level %d\n", gMDNSPlatformPosixVerboseLevel);
            //}
            //if (gReceivedSigHup)
            //{
            //    if (gMDNSPlatformPosixVerboseLevel > 0)
            //        fprintf(stderr, "\nSIGHUP\n");
            //    gReceivedSigHup = mDNSfalse;
            //    DeregisterOurServices();
            //    status = mDNSPlatformPosixRefreshInterfaceList(&mDNSStorage);
            //    if (status != mStatus_NoError) break;
            //    status = RegisterOurServices();
            //    if (status != mStatus_NoError) break;
            //}
        }
    } else {
        mDNSPosixProcessFDSet(&object->dns_storage, &readfds);
    }
    ILibLifeTime_AddEx(object->process_timer, object, 200, ILibDnssdLoop, NULL);
}

void ILibDnssdPreSelect(void * object,void * readset, void * writeset, void * errorset, int * blocktime)
{
    struct DnssdObject *s   = (struct DnssdObject *)object;
    ILibDnssdLoop(s);
    s->terminate            = 0;
    s->pre_select = NULL;
}

void ILibDnssdDestroy(void * object)
{
    struct DnssdObject * s  = (struct DnssdObject *)object;

    freesafe(s->fcr_name);
    freesafe(s->fcr_type);
    freesafe(s->service_domain);

    mDNS_DeregisterService(&s->dns_storage, &s->core_serv);
    mDNS_Close(&s->dns_storage);
}

static void registration_callback(mDNS *const m, ServiceRecordSet *const thisRegistration, mStatus status)
{
    switch (status) {

case mStatus_NoError:      
    debugf("Callback: %##s Name Registered",   thisRegistration->RR_SRV.resrec.name->c); 
    break;

case mStatus_NameConflict: 
    debugf("Callback: %##s Name Conflict",     thisRegistration->RR_SRV.resrec.name->c); 
    status = mDNS_RenameAndReregisterService(m, thisRegistration, mDNSNULL);
    break;

case mStatus_MemFree:      
    debugf("Callback: %##s Memory Free",       thisRegistration->RR_SRV.resrec.name->c); 
    printf("Callback: %s Memory Free",       thisRegistration->RR_SRV.resrec.name->c); 
    break;

default:                   
    debugf("Callback: %##s Unknown Status %ld", thisRegistration->RR_SRV.resrec.name->c, status); 
    break;
    }
}

mStatus register_our_services(struct DnssdObject * object)
{
    mStatus status;

    status = mStatus_NoError;
    if (object->fcr_name[0] != 0) {
        domainlabel         name;
        domainname          type;
        domainname          domain;

        if (status == mStatus_NoError) {
            MakeDomainLabelFromLiteralString(&name,  object->fcr_name);
            MakeDomainNameFromDNSNameString(&type, object->fcr_type);
            MakeDomainNameFromDNSNameString(&domain, object->service_domain);
            status = mDNS_RegisterService(&object->dns_storage, &object->core_serv,
                &name, &type, &domain,
                NULL, mDNSOpaque16fromIntVal(object->f_port),
                object->service_text, object->service_text_len,
                NULL, 0,
                mDNSInterface_Any,
                registration_callback, NULL, 0);
        }
    }
    return status;
}

int ILibDaemonIsRunning()
{
    return 1;
}

void * ILibCreateDnssdModule(void * chain,
                             char * fcr_type,
                             char * fcr_name,
                             unsigned short f_port,
                             void * txt_map,
                             void (* OnDnssdStart)(int error_code, void * user),
                             void * user)
{
    struct DnssdObject * ret_val    = (struct DnssdObject *)malloc(sizeof(struct DnssdObject));
    void * iter                     = NULL;
    char * key                      = NULL;
    char tbuf[128]                  = {0};
    int  len                        = 0;
    char * val                      = NULL;
    mStatus status                  = mStatus_NoError;

    memset(ret_val, 0, sizeof(struct DnssdObject));
    ret_val->pre_select             = &ILibDnssdPreSelect;
    ret_val->post_select            = NULL;
    ret_val->destroy                = &ILibDnssdDestroy;

    status = mDNS_Init(&ret_val->dns_storage, &ret_val->platform_storage,
        mDNS_Init_NoCache, mDNS_Init_ZeroCacheSize,
        mDNS_Init_AdvertiseLocalAddresses,
        mDNS_Init_NoInitCallback, mDNS_Init_NoInitCallbackContext);
    if (status != mStatus_NoError) {
        free(ret_val);
        return NULL;
    }

    ret_val->fcr_name               = CreateString(fcr_name);
    ret_val->fcr_type               = CreateString(fcr_type);
    ret_val->service_domain         = CreateString("local.");
    ret_val->f_port                 = f_port;
    ret_val->terminate              = 1;
    ret_val->OnDnssdStartCallback   = OnDnssdStart;

    iter = ILibHashTree_GetEnumerator(txt_map);
    while ( !ILibHashTree_MoveNext( iter ) ) { // 注意：多一次指针引用，使用完毕由内部释放，外部创建不用释放
        ILibHashTree_GetValue( iter, &key, &len, ((void **)(&val)));
        len = sprintf(tbuf, "%s=%s", (char *)key, (char *)val);
        ret_val->service_text[ret_val->service_text_len] = len;
        mDNSPlatformMemCopy(ret_val->service_text + ret_val->service_text_len + 1, tbuf, ret_val->service_text[ret_val->service_text_len]);
        ret_val->service_text_len += 1 + ret_val->service_text[ret_val->service_text_len];
        freesafe(val);
    }
    ILibHashTree_DestroyEnumerator(iter);

    ret_val->process_timer          = ILibCreateLifeTime(chain);
    status = register_our_services(ret_val);
    if (status != mStatus_NoError) {
        freesafe(ret_val->fcr_name);
        freesafe(ret_val->fcr_type);
        freesafe(ret_val->service_domain);
        free(ret_val);
        return NULL;
    }

    ILibAddToChain(chain, (void *)ret_val);

    return (void *)ret_val;
}

#endif
#endif // ENABLED_AIRPLAY
