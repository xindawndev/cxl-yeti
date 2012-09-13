#include "ILibParsers.h"
#include "dns_sd.h"
#include "ILibDnssd.h"

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

#define CreateString(x)        (char *)strcpy((char *)malloc(strlen(x) + 1), x)

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