#include "AirplayRender.h"
#include "ILibAsyncServerSocket.h"

struct AirplayDataObject {
    ILibChain_PreSelect     pre_select;
    ILibChain_PostSelect    post_select;
    ILibChain_Destroy       destroy;

    void *                  http_server;
    unsigned short          server_port;
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
    char *txt;
    if(header!=NULL && sender->User3==NULL && done==0)
    {
        sender->User3 = (void*)~0;
        txt = ILibGetHeaderLine(header,"Expect",6);
        if(txt!=NULL)
        {
            if(strcasecmp(txt,"100-Continue")==0)
            {
                //
                // Expect Continue
                //
                ILibWebServer_Send_Raw(sender,"HTTP/1.1 100 Continue\r\n\r\n",25,ILibAsyncSocket_MemoryOwnership_STATIC,0);
            }
            else
            {
                //
                // Don't understand
                //
                ILibWebServer_Send_Raw(sender,"HTTP/1.1 417 Expectation Failed\r\n\r\n",35,ILibAsyncSocket_MemoryOwnership_STATIC,1);
                ILibWebServer_DisconnectSession(sender);
                return;
            }
        }
    }

    if(header!=NULL && done !=0 && InterruptFlag==0)
    {
        AirplayProcessHTTPPacket(sender,header,bodyBuffer,beginPointer==NULL?0:*beginPointer,endPointer);
        if(beginPointer!=NULL) {*beginPointer = endPointer;}
    }
}

void AirplaySessionSink(struct ILibWebServer_Session * SessionToken, void * user)
{
    SessionToken->OnReceive = &AirplaySessionReceiveSink;
    SessionToken->User      = user;
}

AirplayToken AirplayCreate(void * chain, const unsigned short port, const char * password)
{
    struct AirplayDataObject * ret_val = (struct AirplayDataObject *)malloc(sizeof(struct AirplayDataObject));
    memset(ret_val, 0, sizeof(struct AirplayDataObject));
    ret_val->pre_select     = NULL;
    ret_val->post_select    = NULL;
    ret_val->destroy        = NULL;

    ret_val->http_server = ILibWebServer_Create(chain, 5, port, &AirplaySessionSink, ret_val);
}
