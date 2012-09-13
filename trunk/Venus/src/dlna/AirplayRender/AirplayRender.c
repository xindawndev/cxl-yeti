#include "ILibParsers.h"
#include "AirplayRender.h"
#include "ILibWebServer.h"
#include "ILibDnssd.h"

struct AirplayDataObject
{
    ILibChain_PreSelect     pre_select;
    ILibChain_PostSelect    post_select;
    ILibChain_Destroy       destroy;

    void *                  txt_map;
    void *                  http_server;
    void *                  dnssd_module;
    unsigned short          server_port;
    char *                  friendly_name;
    char *                  mac_addr;
    char *                  password;
};

void AirplayProcessHTTPPacket()
{
    
}

void AirplaySessionReceiveSink(
struct ILibWebServer_Session * sender,
    int InterruptFlag,
struct packetheader * header,
    char * bodyBuffer,
    int * beginPointer,
    int endPointer,
    int done)
{
    char * txt;
    if (header != NULL && sender->User3 == NULL && done == 0) {
        sender->User3 = (void *)~0;
        txt = ILibGetHeaderLine(header, "Expect", 6);
        if (txt != NULL) {
            if (strcasecmp(txt, "100-Continue") == 0) {
                ILibWebServer_Send_Raw(sender, "HTTP/1.1 100 Continue\r\n\r\n", 25, ILibAsyncSocket_MemoryOwnership_STATIC, 0);
            } else {
                ILibWebServer_Send_Raw(sender, "HTTP/1.1 417 Expectation Failed\r\n\r\n", 35, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
                ILibWebServer_DisconnectSession(sender);
                return;
            }
        }
    }

    if (header != NULL && done != 0 && InterruptFlag == 0) {
        AirplayProcessHTTPPacket(sender, header, bodyBuffer, beginPointer == NULL ? 0: *beginPointer, endPointer);
        if (beginPointer != NULL) { *beginPointer = endPointer; }
    }
}

void AirplaySessionSink(struct ILibWebServer_Session * SessionToken, void * user)
{
    SessionToken->OnReceive = &AirplaySessionReceiveSink;
    SessionToken->User      = user;
}

void AirplayStart(struct AirplayDataObject * object)
{
}

void AirplayPreSelect(void * object,void * readset, void * writeset, void * errorset, int * blocktime)
{
    struct AirplayDataObject *s = (struct AirplayDataObject *)object;
    AirplayStart(s);
    s->pre_select = NULL;
}

void AirplayDestroy(void * object)
{
    struct AirplayDataObject *s = (struct AirplayDataObject *)object;

    freesafe(s->friendly_name);
    freesafe(s->mac_addr);
    freesafe(s->password);
    // map 内部数据交给dnssd销毁
    ILibDestroyHashTree(s->txt_map);
}

void AirplayOnDnssdStart(int error_code, void * user)
{

}

#define CreateString(x)        (char *)strcpy((char *)malloc(strlen(x) + 1), x)

AirplayToken AirplayCreate(void * chain,
                           const unsigned short port,
                           const char * friendly_name,
                           const char * mac_addr,
                           const char * password)
{
    char * key = NULL;
    char * val = NULL;
    struct AirplayDataObject * ret_val = NULL;

    if (ILibDaemonIsRunning() == 0) {
        return NULL; // Daemon没有启动
    }

    ret_val = (struct AirplayDataObject *)malloc(sizeof(struct AirplayDataObject));
    memset(ret_val, 0, sizeof(struct AirplayDataObject));
    ret_val->pre_select     = &AirplayPreSelect;
    ret_val->post_select    = NULL;
    ret_val->destroy        = &AirplayDestroy;
    ret_val->friendly_name  = CreateString(friendly_name);
    ret_val->mac_addr       = CreateString(mac_addr);
    if (password != NULL) {
        ret_val->password   = CreateString(password);
    }

    ret_val->txt_map        = ILibInitHashTree();

    val = CreateString(mac_addr);
    ILibAddEntry(ret_val->txt_map, "deviceid", strlen("deviceid"), ( void * )val );
    val = CreateString("0x77");
    ILibAddEntry(ret_val->txt_map, "features", strlen("features"), ( void * )val );
    val = CreateString("AppleTV2,1");
    ILibAddEntry(ret_val->txt_map, "model", strlen("model"), ( void * )val );
    val = CreateString("101.28");
    ILibAddEntry(ret_val->txt_map, "srcvers", strlen("srcvers"), ( void * )val );

    ret_val->http_server    = ILibWebServer_Create(chain, 5, port, &AirplaySessionSink, ret_val);
    ret_val->dnssd_module   = ILibCreateDnssdModule(chain, "_airplay._tcp", ret_val->friendly_name, port, ret_val->txt_map, AirplayOnDnssdStart, ret_val);
    ILibAddToChain(chain, (void *)ret_val);

    return (void *)ret_val;
}
