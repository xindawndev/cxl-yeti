#include "ILibParsers.h"
#include "AirplayRender.h"
#include "ILibWebServer.h"
#include "ILibDnssd.h"
#include "ILibMd5.h"

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK                  200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NEED_AUTH           401
#define AIRPLAY_STATUS_NOT_FOUND           404
#define AIRPLAY_STATUS_METHOD_NOT_ALLOWED  405
#define AIRPLAY_STATUS_NOT_IMPLEMENTED     501
#define AIRPLAY_STATUS_NO_RESPONSE_NEEDED  1000

#define EVENT_NONE     -1
#define EVENT_PLAYING   0
#define EVENT_PAUSED    1
#define EVENT_LOADING   2
#define EVENT_STOPPED   3
const char *eventStrings[] = {"playing", "paused", "loading", "stopped"};

#define PLAYBACK_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
    "<plist version=\"1.0\">\r\n"\
    "<dict>\r\n"\
    "<key>duration</key>\r\n"\
    "<real>%f</real>\r\n"\
    "<key>loadedTimeRanges</key>\r\n"\
    "<array>\r\n"\
    "\t\t<dict>\r\n"\
    "\t\t\t<key>duration</key>\r\n"\
    "\t\t\t<real>%f</real>\r\n"\
    "\t\t\t<key>start</key>\r\n"\
    "\t\t\t<real>0.0</real>\r\n"\
    "\t\t</dict>\r\n"\
    "</array>\r\n"\
    "<key>playbackBufferEmpty</key>\r\n"\
    "<true/>\r\n"\
    "<key>playbackBufferFull</key>\r\n"\
    "<false/>\r\n"\
    "<key>playbackLikelyToKeepUp</key>\r\n"\
    "<true/>\r\n"\
    "<key>position</key>\r\n"\
    "<real>%f</real>\r\n"\
    "<key>rate</key>\r\n"\
    "<real>%d</real>\r\n"\
    "<key>readyToPlay</key>\r\n"\
    "<true/>\r\n"\
    "<key>seekableTimeRanges</key>\r\n"\
    "<array>\r\n"\
    "\t\t<dict>\r\n"\
    "\t\t\t<key>duration</key>\r\n"\
    "\t\t\t<real>%f</real>\r\n"\
    "\t\t\t<key>start</key>\r\n"\
    "\t\t\t<real>0.0</real>\r\n"\
    "\t\t</dict>\r\n"\
    "</array>\r\n"\
    "</dict>\r\n"\
    "</plist>\r\n"

#define PLAYBACK_INFO_NOT_READY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
    "<plist version=\"1.0\">\r\n"\
    "<dict>\r\n"\
    "<key>readyToPlay</key>\r\n"\
    "<false/>\r\n"\
    "</dict>\r\n"\
    "</plist>\r\n"

#define SERVER_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
    "<plist version=\"1.0\">\r\n"\
    "<dict>\r\n"\
    "<key>deviceid</key>\r\n"\
    "<string>%s</string>\r\n"\
    "<key>features</key>\r\n"\
    "<integer>119</integer>\r\n"\
    "<key>model</key>\r\n"\
    "<string>AppleTV2,1</string>\r\n"\
    "<key>protovers</key>\r\n"\
    "<string>1.0</string>\r\n"\
    "<key>srcvers</key>\r\n"\
    "<string>"AIRPLAY_SERVER_VERSION_STR"</string>\r\n"\
    "</dict>\r\n"\
    "</plist>\r\n"

#define EVENT_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
    "<plist version=\"1.0\">\r\n"\
    "<dict>\r\n"\
    "<key>category</key>\r\n"\
    "<string>video</string>\r\n"\
    "<key>state</key>\r\n"\
    "<string>%s</string>\r\n"\
    "</dict>\r\n"\
    "</plist>\r\n"\

#define AUTH_REALM "AirPlay"
#define AUTH_REQUIRED "WWW-Authenticate: Digest realm=\""  AUTH_REALM  "\", nonce=\"%s\"\r\n"

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

char * AirplayCalcResponse(const char * username,
                          const char * password,
                          const char * realm,
                          const char * method,
                          const char * digestUri,
                          const char * nonce)
{
    char * resp;
    char * ha1;
    char * ha2;
    char buf[128] = {0};
    char * bytes = NULL;

    MD5_CTX md5ctx;

    MD5Init(&md5ctx, 0);
    sprintf(buf, "%s:%s:%s", username, realm, password);
    MD5Update(&md5ctx, buf, strlen(buf));
    MD5Final(&md5ctx);
    bytes = (char *)&(md5ctx.digest);

    //ha1 = Md5::GetMD5(username + ":" + realm + ":" + password);
    //ha2 = Md5::GetMD5(method + ":" + digestUri);

    //resp = Md5::GetMD5(str_to_lower(ha1) + ":" + nonce + ":" + str_to_lower(ha2));
    //return str_to_lower(resp);

    return NULL;
}

char * AirplayGetFildFromString(const char * auth_str, const char * field)
{
    char * retstr = NULL;
    //int field_index = ILibString_IndexOf(auth_str, strlen(auth_str), field, strlen(field));
    //if (field_index != -1) {
    //    int equal_index = ILibString_IndexOf(auth_str + field_index, strlen(auth_str) - field_index, "=", 1);
    //}
    return retstr;
}

int AirplayCheckAuthorization(void * object, char * auth_str, char * method, char * uri)
{
    struct AirplayDataObject *s = (struct AirplayDataObject *)object;
    int auth_valid              = 1;
    char * username             = NULL;

    if (auth_str == NULL) {
        return 0;
    }

    //username = AirplayGetFildFromString(auth_str, "username");

    //if (username == NULL) {
    //    auth_valid = 0;
    //}

    //if (auth_valid)
    //    if (AirplayGetFildFromString(auth_str, "realm") != AUTH_REALM)
    //        auth_valid = 0;

    //if (auth_valid)
    //    if (AirplayGetFildFromString(auth_str, "nonce") != m_auth_nonce_)
    //        auth_valid = 0;

    //if (auth_valid)
    //    if (AirplayGetFildFromString(auth_str, "uri") != uri)
    //        auth_valid = 0;

    //if (auth_valid) {
    //    char *  realm = AUTH_REALM;
    //    char * our_resp = AirplayCalcResponse(username, server_instance_->m_pwd_, realm, method, uri, m_auth_nonce_);
    //    char * their_resp = AirplayGetFildFromString(auth_str, "response");

    //    std::string tmp1 = their_resp, tmp2 = our_resp;
    //    if (str_to_lower(tmp1) != str_to_lower(tmp2)) { // 需要大小写不敏感比较
    //        auth_valid = 0;
    //        printf("AirAuth: response mismatch - our: %s theirs: %s\n", our_resp.c_str(), their_resp.c_str());
    //    } else {
    //        printf("AirAuth: successfull authentication from AirPlay client\n");
    //    }
    //}

    return auth_valid;
}

void AirplayProcessHTTPPacket(struct ILibWebServer_Session * session, struct packetheader * header, char * bodyBuffer, int offset, int bodyBufferLength)
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
    (void)user;
    printf("Start dns_sd ret = %d\n", error_code);
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
