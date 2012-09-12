#if defined(WINSOCK2)
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(WINSOCK1)
#include <winsock.h>
#include <wininet.h>
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
#define _CRTDBG_MAP_ALLOC
#endif

#include "ILibParsers.h"
#include "ILibSSDPClient.h"
#include "ILibAsyncUDPSocket.h"

#ifndef _WIN32_WCE
#include <time.h>
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
#include <crtdbg.h>
#endif

#define UPNP_PORT 1900
#define UPNP_GROUP "239.255.255.250"
#define DEBUGSTATEMENT(x)

struct SSDPClientModule
{
    ILibChain_PreSelect PreSelect;
    ILibChain_PostSelect PostSelect;
    ILibChain_Destroy Destroy;
    void (*FunctionCallback)(void *sender, char* UDN, int Alive, char* LocationURL, int Timeout, UPnPSSDP_MESSAGE m, void *user);
    char* DeviceURN;
    int DeviceURNLength;

    char* DeviceURN_Prefix;
    int DeviceURN_PrefixLength;
    int BaseDeviceVersionNumber;

    int *IPAddress;
    int NumIPAddress;

    void* IP_SSDP_Mointor;

    void* SSDPListenSocket;
    void* MSEARCH_Response_Socket;

    int Terminate;
    void *Reserved;
};

void ILibReadSSDP(struct packetheader *packet,int remoteInterface, unsigned short remotePort, struct SSDPClientModule *module)
{
    struct packetheader_field_node *node;
    struct parser_result *pnode,*pnode2;
    struct parser_result_field *prf;

    char* Location = NULL;
    char* UDN = NULL;
    int Timeout = 0;
    int Alive = 0;
    int OK;
    int rt;

    char *IP;
    unsigned short PORT;
    char *PATH;
    int MATCH=0;

    if(packet->Directive==NULL)
    {
        /* M-SEARCH Response */
        if(packet->StatusCode==200)
        {
            node = packet->FirstField;
            while(node!=NULL)
            {

                if(strncasecmp(node->Field,"LOCATION",8)==0)
                {
                    Location = node->FieldData;
                    Location[node->FieldDataLength] = 0;
                    //Location = (char*)malloc(node->FieldDataLength+1);
                    //memcpy(Location,node->FieldData,node->FieldDataLength);
                    //Location[node->FieldDataLength] = '\0';
                }
                if(strncasecmp(node->Field,"CACHE-CONTROL",13)==0)
                {
                    pnode = ILibParseString(node->FieldData, 0, node->FieldDataLength, ",", 1);
                    prf = pnode->FirstResult;
                    while(prf!=NULL)
                    {
                        pnode2 = ILibParseString(prf->data, 0, prf->datalength, "=", 1);
                        pnode2->FirstResult->datalength = ILibTrimString(&(pnode2->FirstResult->data),pnode2->FirstResult->datalength);
                        pnode2->FirstResult->data[pnode2->FirstResult->datalength]=0;
                        if(strcasecmp(pnode2->FirstResult->data,"max-age")==0)
                        {
                            pnode2->LastResult->datalength = ILibTrimString(&(pnode2->LastResult->data),pnode2->LastResult->datalength);
                            pnode2->LastResult->data[pnode2->LastResult->datalength]=0;
                            Timeout = atoi(pnode2->LastResult->data);
                            ILibDestructParserResults(pnode2);
                            break;
                        }
                        prf = prf->NextResult;
                        ILibDestructParserResults(pnode2);
                    }
                    ILibDestructParserResults(pnode);
                }
                if(strncasecmp(node->Field,"USN",3)==0)
                {
                    pnode = ILibParseString(node->FieldData, 0, node->FieldDataLength, "::", 2);
                    pnode->FirstResult->data[pnode->FirstResult->datalength] = '\0';
                    UDN = pnode->FirstResult->data+5;
                    ILibDestructParserResults(pnode);
                }
                node = node->NextField;
            }
            ILibParseUri(Location,&IP,&PORT,&PATH);
            if(remoteInterface==inet_addr(IP))
            {

                if(module->FunctionCallback!=NULL)
                {
                    module->FunctionCallback(module,UDN,-1,Location,Timeout,UPnPSSDP_MSEARCH,module->Reserved);
                }
            }
            free(IP);
            free(PATH);
        }
    }
    else
    {
        /* Notify Packet */
        if(strncasecmp(packet->Directive,"NOTIFY",6)==0)
        {
            OK = 0;
            rt = 0;
            node = packet->FirstField;

            while(node!=NULL)
            {
                node->Field[node->FieldLength] = '\0';
                if(strncasecmp(node->Field,"NT",2)==0 && node->FieldLength==2)
                {
                    node->FieldData[node->FieldDataLength] = '\0';
                    if(strncasecmp(node->FieldData,module->DeviceURN_Prefix,module->DeviceURN_PrefixLength)==0)
                    {


                        if(atoi(node->FieldData+module->DeviceURN_PrefixLength)>=module->BaseDeviceVersionNumber)
                        {
                            OK = -1;
                        }

                    }

                    else if(strncasecmp(node->FieldData,"upnp:rootdevice",15)==0)
                    {
                        rt = -1;
                    }

                }
                if(strncasecmp(node->Field,"NTS",3)==0)
                {
                    if(strncasecmp(node->FieldData,"ssdp:alive",10)==0)
                    {
                        Alive = -1;
                        rt = 0;
                    }
                    else
                    {
                        Alive = 0;
                        OK = 0;
                    }
                }
                if(strncasecmp(node->Field,"USN",3)==0)
                {
                    pnode = ILibParseString(node->FieldData, 0, node->FieldDataLength, "::", 2);
                    pnode->FirstResult->data[pnode->FirstResult->datalength] = '\0';
                    UDN = pnode->FirstResult->data+5;
                    ILibDestructParserResults(pnode);
                }
                if(strncasecmp(node->Field,"LOCATION",8)==0)
                {
                    Location = node->FieldData;
                    Location[node->FieldDataLength] = 0;
                }

                if(strncasecmp(node->Field,"CACHE-CONTROL",13)==0)
                {
                    pnode = ILibParseString(node->FieldData, 0, node->FieldDataLength, ",", 1);
                    prf = pnode->FirstResult;
                    while(prf!=NULL)
                    {
                        pnode2 = ILibParseString(prf->data, 0, prf->datalength, "=", 1);
                        pnode2->FirstResult->datalength = ILibTrimString(&(pnode2->FirstResult->data),pnode2->FirstResult->datalength);
                        pnode2->FirstResult->data[pnode2->FirstResult->datalength]=0;
                        if(strcasecmp(pnode2->FirstResult->data,"max-age")==0)
                        {
                            pnode2->LastResult->datalength = ILibTrimString(&(pnode2->LastResult->data),pnode2->LastResult->datalength);
                            pnode2->LastResult->data[pnode2->LastResult->datalength]=0;
                            Timeout = atoi(pnode2->LastResult->data);
                            ILibDestructParserResults(pnode2);
                            break;
                        }
                        prf = prf->NextResult;
                        ILibDestructParserResults(pnode2);
                    }
                    ILibDestructParserResults(pnode);
                }
                node = node->NextField;
            }
            if((OK!=0 && Alive!=0) || (Alive==0)) 
            {
                if(Location!=NULL)
                {
                    ILibParseUri(Location,&IP,&PORT,&PATH);
                    if(remoteInterface==inet_addr(IP))
                    {
                        MATCH=1;
                    }
                    else
                    {
                        MATCH=0;
                    }
                    free(IP);
                    free(PATH);

                }
                if(Alive==0 || MATCH!=0)
                {
                    if(module->FunctionCallback!=NULL)
                    {
                        module->FunctionCallback(module,UDN,Alive,Location,Timeout,UPnPSSDP_NOTIFY,module->Reserved);
                    }
                }
            }

        }
    }
}

void ILibSSDPClient_OnData(ILibAsyncUDPSocket_SocketModule socketModule,char* buffer, int bufferLength, int remoteInterface, unsigned short remotePort, void *user, void *user2, int *PAUSE)
{
    struct packetheader *packet;
    packet = ILibParsePacketHeader(buffer,0,bufferLength);
    if(packet==NULL) {return;}

    ILibReadSSDP(packet,remoteInterface,remotePort,(struct SSDPClientModule*)user);
    ILibDestructPacket(packet);
}

void ILibSSDPClientModule_Destroy(void *object)
{
    struct SSDPClientModule *s = (struct SSDPClientModule*)object;

    free(s->DeviceURN);
    if(s->IPAddress!=NULL)
    {
        free(s->IPAddress);
    }
}

void ILibSSDPDeviceSearch(void * SSDPToken)
{
    ILibSSDP_IPAddressListChanged(SSDPToken);
}

void ILibSSDP_IPAddressListChanged(void *SSDPToken)
{
    struct SSDPClientModule *RetVal = (struct SSDPClientModule*)SSDPToken;
    int i;
    char* buffer;
    int bufferlength;

    if(RetVal->IPAddress!=NULL)
    {
        free(RetVal->IPAddress);
    }
    RetVal->NumIPAddress = ILibGetLocalIPAddressList(&(RetVal->IPAddress));

    for(i=0;i<RetVal->NumIPAddress;++i)
    {
        if ( !RetVal->SSDPListenSocket ) continue;//leochen
        ILibAsyncUDPSocket_JoinMulticastGroup(RetVal->SSDPListenSocket, RetVal->IPAddress[i], inet_addr(UPNP_GROUP));
    }

    buffer = (char*)malloc(105+RetVal->DeviceURNLength);
    bufferlength = sprintf(buffer,"M-SEARCH * HTTP/1.1\r\nMX: 3\r\nST: %s\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\n\r\n",RetVal->DeviceURN);

    for(i=0;i<RetVal->NumIPAddress;++i)
    {
#if defined(_WIN32_WCE) && _WIN32_WCE < 400
        ILibAsyncUDPSocket_SendTo(RetVal->MSEARCH_Response_Socket, inet_addr(UPNP_GROUP), UPNP_PORT, buffer, bufferlength, ILibAsyncSocket_MemoryOwnership_USER);
#else
        if(ILibAsyncUDPSocket_SetMulticastInterface(RetVal->MSEARCH_Response_Socket, RetVal->IPAddress[i]) == 0)
        {
            ILibAsyncUDPSocket_SendTo(RetVal->MSEARCH_Response_Socket, inet_addr(UPNP_GROUP), UPNP_PORT, buffer, bufferlength, ILibAsyncSocket_MemoryOwnership_USER);
        }
#endif
    }
    free(buffer);
}

void ILibSSDPSearchTimer(void * SSDPToken)
{
    struct SSDPClientModule *RetVal = (struct SSDPClientModule *)SSDPToken;
    int u;
    // 产生一个[10, 20)秒的随机值
    u = (int)((double) rand() / (RAND_MAX + 1) * (20 - 10) + 10);
    ILibLifeTime_Add( RetVal->IP_SSDP_Mointor, SSDPToken, u, (void*)&ILibSSDP_IPAddressListChanged, NULL );
}

void ILibSSDPClientModule_PreSelect(void* object,void *readset, void *writeset, void *errorset, int* blocktime)
{
    struct SSDPClientModule *s = (struct SSDPClientModule*)object;
    s->PreSelect = NULL;
    ILibSSDP_IPAddressListChanged(object);
    ILibSSDPSearchTimer(object);
}

void* ILibCreateSSDPClientModule(void *chain, char* DeviceURN, int DeviceURNLength, void (*CallbackPtr)(void *sender, char* UDN, int Alive, char* LocationURL, int Timeout, UPnPSSDP_MESSAGE m,void *user),void *user)
{
    struct SSDPClientModule *RetVal = (struct SSDPClientModule*)malloc(sizeof(struct SSDPClientModule));
    unsigned char TTL = 4;
    struct parser_result *pr;

    RetVal->Destroy                                     = &ILibSSDPClientModule_Destroy;
    RetVal->PreSelect                                   = &ILibSSDPClientModule_PreSelect;
    RetVal->PostSelect                                  = NULL;
    RetVal->Reserved                                    = user;
    RetVal->Terminate                                   = 0;
    RetVal->FunctionCallback                            = CallbackPtr;
    RetVal->DeviceURN                                   = (char*)malloc(DeviceURNLength+1);
    memcpy(RetVal->DeviceURN,DeviceURN,DeviceURNLength);
    RetVal->DeviceURN[DeviceURNLength]                  = '\0';
    RetVal->DeviceURNLength                             = DeviceURNLength;

    // Populate the Prefix portion of the URN, for matching purposes
    RetVal->DeviceURN_Prefix                            = RetVal->DeviceURN;
    pr = ILibParseString(RetVal->DeviceURN,0,RetVal->DeviceURNLength,":",1);
    RetVal->DeviceURN_PrefixLength                      = (int)((pr->LastResult->data)-(RetVal->DeviceURN));
    pr->LastResult->data[pr->LastResult->datalength]    = 0;
    RetVal->BaseDeviceVersionNumber                     = atoi(pr->LastResult->data);
    ILibDestructParserResults(pr);

    RetVal->IPAddress                                   = NULL;

    RetVal->IP_SSDP_Mointor                             = ILibCreateLifeTime( chain );

    RetVal->SSDPListenSocket                            = ILibAsyncUDPSocket_Create(chain, 4096, 0, 1900, ILibAsyncUDPSocket_Reuse_SHARED, ILibSSDPClient_OnData , NULL, RetVal);
    RetVal->MSEARCH_Response_Socket                     = ILibAsyncUDPSocket_Create(chain, 4096, 0, 0, ILibAsyncUDPSocket_Reuse_EXCLUSIVE, ILibSSDPClient_OnData , NULL, RetVal);

    ILibAddToChain(chain,RetVal);
    ILibAsyncUDPSocket_SetMulticastTTL(RetVal->MSEARCH_Response_Socket, TTL);

    return(RetVal);
}
