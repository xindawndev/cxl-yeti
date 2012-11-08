/*****************************************************************
|
|   Neptune - Sockets :: WinRT Implementation
|
|   (c) 2001-2012 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptWinRtPch.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Concurrency;

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.sockets.winrt")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const DWORD NPT_WINRT_SOCKET_DEFAULT_READ_TIMEOUT  = 30000;
const DWORD NPT_WINRT_SOCKET_DEFAULT_WRITE_TIMEOUT = 30000;

/*----------------------------------------------------------------------
|   StringFromUTF8
+---------------------------------------------------------------------*/
static String^ 
    StringFromUTF8(const char* utf)
{
    unsigned int utf_len = NPT_StringLength(utf);
    unsigned int wide_len = utf_len;
    wchar_t* wide = new wchar_t[wide_len+1];
    int result = MultiByteToWideChar(CP_UTF8,
        0,
        utf,
        utf_len+1,
        wide,
        wide_len+1);
    String^ str;
    if (result) {
        str = ref new String(wide);
    } else {
        str = ref new String();
    }
    delete[] wide;
    return str;
}

/*----------------------------------------------------------------------
|   TranslateHResult
+---------------------------------------------------------------------*/
static NPT_Result
    TranslateHResult(HResult result)
{
    switch (HRESULT_FACILITY(result.Value)) {
    case FACILITY_WIN32:
        switch (HRESULT_CODE(result.Value)) {
        case WSAHOST_NOT_FOUND:
            return NPT_ERROR_HOST_UNKNOWN;

        case WSAETIMEDOUT:
            return NPT_ERROR_TIMEOUT;

        case WSAECONNREFUSED:
            return NPT_ERROR_CONNECTION_REFUSED;

        case WSAEWOULDBLOCK:
            return NPT_ERROR_WOULD_BLOCK;

        case WSAECONNABORTED:
            return NPT_ERROR_CONNECTION_ABORTED;

        case WSAECONNRESET:
        case WSAENETRESET:
            return NPT_ERROR_CONNECTION_RESET;

        case WSAEADDRINUSE:
            return NPT_ERROR_ADDRESS_IN_USE;

        case WSAENETDOWN:
            return NPT_ERROR_NETWORK_DOWN;

        case WSAENETUNREACH:
            return NPT_ERROR_NETWORK_UNREACHABLE;

        case WSAEINTR:
            return NPT_ERROR_INTERRUPTED;

        case WSAENOTCONN:
            return NPT_ERROR_NOT_CONNECTED;

        default:
            return NPT_FAILURE;
        }
        break;

        /* TODO: map error codes */
    default:
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   WaitForAsyncAction
+---------------------------------------------------------------------*/
static NPT_Result
    WaitForAsyncAction(IAsyncAction^ action, 
    HANDLE        wait_event, 
    DWORD         timeout = INFINITE)
{
    NPT_Result result = NPT_ERROR_INTERNAL;

    NPT_LOG_FINEST("waiting for async action...");
    ResetEvent(wait_event);

    action->Completed = ref new AsyncActionCompletedHandler
        ([&](IAsyncAction^ action_, AsyncStatus status) {
            switch (status) {
            case AsyncStatus::Canceled:
                result = NPT_ERROR_TIMEOUT;
                break;

            case AsyncStatus::Completed:
                result = NPT_SUCCESS;
                break;

            case AsyncStatus::Error:
                NPT_LOG_FINE_1("AsyncAction error %x", action_->ErrorCode.Value);
                result = TranslateHResult(action_->ErrorCode);
                break;

            default:
                result = NPT_ERROR_INTERNAL;
                break;
            }
            SetEvent(wait_event);
    });

    DWORD wait_result = WaitForSingleObjectEx(wait_event, timeout, FALSE);
    if (wait_result != WAIT_OBJECT_0) {
        NPT_LOG_FINE("action timed out, canceling...");
        action->Cancel();
        WaitForSingleObjectEx(wait_event, INFINITE, FALSE);
    }
    NPT_LOG_FINEST("done waiting for async action");

    return result;
}

/*----------------------------------------------------------------------
|   WaitForAsyncOperation
+---------------------------------------------------------------------*/
static NPT_Result
    WaitForAsyncOperation(IAsyncOperation<unsigned int>^ operation, 
    HANDLE                         wait_event,
    unsigned int&                  return_value,
    DWORD                          timeout = INFINITE)
{
    NPT_Result result = NPT_ERROR_INTERNAL;

    NPT_LOG_FINEST("waiting for async operation...");
    return_value = 0;
    ResetEvent(wait_event);

    operation->Completed = ref new AsyncOperationCompletedHandler<unsigned int> 
        ([&](IAsyncOperation<unsigned int>^ operation_, AsyncStatus status) {
            switch (status) {
            case AsyncStatus::Canceled:
                result = NPT_ERROR_TIMEOUT;
                break;

            case AsyncStatus::Completed:
                return_value = operation_->GetResults();
                result = NPT_SUCCESS;
                break;

            case AsyncStatus::Error:
                NPT_LOG_FINE_1("AsyncOperation error %x", operation_->ErrorCode.Value);
                result = TranslateHResult(operation_->ErrorCode);
                break;

            default:
                result = NPT_ERROR_INTERNAL;
                break;
            }
            operation_->Close();
            SetEvent(wait_event);
    });

    DWORD wait_result = WaitForSingleObjectEx(wait_event, timeout, FALSE);
    if (wait_result != WAIT_OBJECT_0) {
        NPT_LOG_FINE("operation timed out, canceling...");
        operation->Cancel();
        WaitForSingleObjectEx(wait_event, INFINITE, FALSE);
    }
    NPT_LOG_FINEST("done waiting for async operation");

    return result;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketInputStream
+---------------------------------------------------------------------*/
class NPT_WinRtSocketInputStream : public NPT_InputStream
{
public:
    // constructors and destructor
    NPT_WinRtSocketInputStream(StreamSocket^ socket, NPT_Timeout timeout);
    virtual ~NPT_WinRtSocketInputStream();

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
        NPT_Size  bytes_to_read, 
        NPT_Size* bytes_read);
    NPT_Result Seek(NPT_Position offset);
    NPT_Result Tell(NPT_Position& where);
    NPT_Result GetSize(NPT_LargeSize& size);
    NPT_Result GetAvailable(NPT_LargeSize& available);

private:
    StreamSocket^ m_Socket;
    IInputStream^ m_InputStream;
    DataReader^   m_Reader;
    HANDLE        m_WaitEvent;
    NPT_Timeout   m_Timeout; 
};

/*----------------------------------------------------------------------
|   NPT_WinRtSocketInputStream::NPT_WinRtSocketInputStream
+---------------------------------------------------------------------*/
NPT_WinRtSocketInputStream::NPT_WinRtSocketInputStream(StreamSocket^ socket, 
                                                       NPT_Timeout   timeout) :
m_Socket(socket),
    m_Timeout(timeout)
{
    m_InputStream = socket->InputStream;
    m_Reader = ref new DataReader(m_InputStream);
    m_Reader->InputStreamOptions = InputStreamOptions::Partial;
    m_WaitEvent = CreateEventExW(NULL, L"", 0, EVENT_ALL_ACCESS);
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketInputStream::~NPT_WinRtSocketInputStream
+---------------------------------------------------------------------*/
NPT_WinRtSocketInputStream::~NPT_WinRtSocketInputStream()
{
    m_Reader->DetachStream();
    CloseHandle(m_WaitEvent);
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result
    NPT_WinRtSocketInputStream::Read(void*     buffer, 
    NPT_Size  bytes_to_read, 
    NPT_Size* bytes_read)
{
    // init and shortcut
    if (bytes_read) *bytes_read = 0;
    if (bytes_to_read == 0) return NPT_SUCCESS;

    NPT_LOG_FINER_1("reading %d bytes", bytes_to_read);
    auto operation = m_Reader->LoadAsync(bytes_to_read);

    unsigned int return_value = 0;
    NPT_Result result = WaitForAsyncOperation(operation, m_WaitEvent, return_value, m_Timeout);

    if (NPT_SUCCEEDED(result)) {
        if (return_value) {
            unsigned int bytes_available = m_Reader->UnconsumedBufferLength;
            Array<unsigned char>^ bytes = ref new Array<unsigned char>(bytes_available);
            m_Reader->ReadBytes(bytes);
            NPT_CopyMemory(buffer, bytes->Data, bytes_available);
            if (bytes_read) *bytes_read = bytes_available;
            return NPT_SUCCESS;
        } else {
            return NPT_ERROR_EOS;
        }
    }

    return result;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketInputStream::Seek
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtSocketInputStream::Seek(NPT_Position offset)
{
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketInputStream::Tell
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtSocketInputStream::Tell(NPT_Position& where)
{
    where = 0;
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtSocketInputStream::GetSize(NPT_LargeSize& size)
{
    size = 0;
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtSocketInputStream::GetAvailable(NPT_LargeSize& available)
{
    available = 0;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketOutputStream
+---------------------------------------------------------------------*/
class NPT_WinRtSocketOutputStream : public NPT_OutputStream
{
public:
    // constructors and destructor
    NPT_WinRtSocketOutputStream(StreamSocket^ socket, NPT_Timeout timeout);
    virtual ~NPT_WinRtSocketOutputStream();

    // NPT_OutputStream methods
    NPT_Result Write(const void* buffer, 
        NPT_Size    bytes_to_write, 
        NPT_Size*   bytes_written);
    NPT_Result Seek(NPT_Position offset);
    NPT_Result Tell(NPT_Position& where);
    NPT_Result Flush();

private:
    StreamSocket^  m_Socket;
    IOutputStream^ m_OutputStream;
    DataWriter^    m_Writer;
    HANDLE         m_WaitEvent;
    NPT_Timeout    m_Timeout;
};

/*----------------------------------------------------------------------
|   NPT_WinRtSocketOutputStream::NPT_WinRtSocketOutputStream
+---------------------------------------------------------------------*/
NPT_WinRtSocketOutputStream::NPT_WinRtSocketOutputStream(StreamSocket^ socket,
                                                         NPT_Timeout   timeout) :
m_Socket(socket),
    m_Timeout(timeout)
{
    m_OutputStream = socket->OutputStream;
    m_Writer = ref new DataWriter(m_OutputStream);
    m_WaitEvent = CreateEventExW(NULL, L"", 0, EVENT_ALL_ACCESS);
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketOutputStream::~NPT_WinRtSocketOutputStream
+---------------------------------------------------------------------*/
NPT_WinRtSocketOutputStream::~NPT_WinRtSocketOutputStream()
{
    m_Writer->DetachStream();
    CloseHandle(m_WaitEvent);
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtSocketOutputStream::Write(const void* buffer, 
    NPT_Size    bytes_to_write, 
    NPT_Size*   bytes_written)
{
    NPT_LOG_FINER_1("writing %d bytes", bytes_to_write);

    Array<unsigned char>^ bytes = ref new Array<unsigned char>(bytes_to_write);
    NPT_CopyMemory(bytes->Data, buffer, bytes_to_write);
    m_Writer->WriteBytes(bytes);
    auto operation = m_Writer->StoreAsync();
    unsigned int return_value = 0;

    NPT_Result result = WaitForAsyncOperation(operation, m_WaitEvent, return_value, m_Timeout);
    if (bytes_written) *bytes_written = return_value;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketOutputStream::Seek
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtSocketOutputStream::Seek(NPT_Position offset)
{
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketOutputStream::Tell
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtSocketOutputStream::Tell(NPT_Position& where)
{
    where = 0;
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_WinRtSocketOutputStream
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtSocketOutputStream::Flush()
{
    return NPT_SUCCESS;
}

class NPT_WinRtTcpSocket : public NPT_SocketInterface
{
public:
    NPT_WinRtTcpSocket(StreamSocket^ sock, NPT_Flags flags)
        : m_TcpSocket(sock) {
            RefreshInfo();
    }
    virtual ~NPT_WinRtTcpSocket() {
        delete m_TcpSocket;
    }

    NPT_Result RefreshInfo() {
        NPT_String localaddr  = Str2NPT_Str(m_TcpSocket->Information->LocalAddress->RawName);
        NPT_String localport  = Str2NPT_Str(m_TcpSocket->Information->LocalPort);
        NPT_String remoteaddr = Str2NPT_Str(m_TcpSocket->Information->RemoteAddress->RawName);
        NPT_String remoteport = Str2NPT_Str(m_TcpSocket->Information->RemotePort);

        NPT_IpAddress addr;
        NPT_UInt32 port;

        addr.Parse(localaddr.GetChars());
        localport.ToInteger32(port);

        m_Info.local_address.SetIpAddress(addr);
        m_Info.local_address.SetPort(port);

        addr.Parse(remoteaddr.GetChars());
        remoteport.ToInteger32(port);

        m_Info.remote_address.SetIpAddress(addr);
        m_Info.remote_address.SetPort(port);
        return NPT_SUCCESS;
    }

    NPT_Result Bind(const NPT_SocketAddress& address, bool reuse_address = true) {
        return NPT_SUCCESS;
    }

    NPT_Result Connect(const NPT_SocketAddress& address, NPT_Timeout timeout) {
        // this is unsupported unless overridden in a derived class
        return NPT_ERROR_NOT_SUPPORTED;
    }

    NPT_Result WaitForConnection(NPT_Timeout timeout) {
        // this is unsupported unless overridden in a derived class
        return NPT_ERROR_NOT_SUPPORTED;
    }

    NPT_Result GetInputStream(NPT_InputStreamReference& stream) {
        stream = new NPT_WinRtSocketInputStream(m_TcpSocket, m_ReadTimeout);
        return NPT_SUCCESS;
    }

    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream) {
        stream = new NPT_WinRtSocketOutputStream(m_TcpSocket, m_WriteTimeout);
        return NPT_SUCCESS;
    }

    NPT_Result GetInfo(NPT_SocketInfo& info) {
        info = m_Info;
        return NPT_SUCCESS;
    }

    NPT_Result SetReadTimeout(NPT_Timeout timeout) {
        m_ReadTimeout = timeout;
        return NPT_SUCCESS;
    }

    NPT_Result SetWriteTimeout(NPT_Timeout timeout) {
        m_WriteTimeout = timeout;
        return NPT_SUCCESS;
    }

    NPT_Result Cancel(bool shutdown) {
        return NPT_SUCCESS;
    }

private:
    StreamSocket^   m_TcpSocket;
    NPT_SocketInfo  m_Info;
    HANDLE          m_WaitEvent;
    NPT_Timeout     m_ReadTimeout;
    NPT_Timeout     m_WriteTimeout;
};

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket
+---------------------------------------------------------------------*/
class NPT_WinRtTcpClientSocket : public NPT_SocketInterface
{
public:
    // constructors and destructor
    NPT_WinRtTcpClientSocket();
    virtual ~NPT_WinRtTcpClientSocket();

    // NPT_SocketInterface methods
    NPT_Result Bind(const NPT_SocketAddress& address, bool reuse_address = true);
    NPT_Result Connect(const NPT_SocketAddress& address, NPT_Timeout timeout);
    NPT_Result WaitForConnection(NPT_Timeout timeout);
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream);
    NPT_Result GetInfo(NPT_SocketInfo& info);
    NPT_Result SetReadTimeout(NPT_Timeout timeout);
    NPT_Result SetWriteTimeout(NPT_Timeout timeout);
    NPT_Result Cancel(bool shutdown);

protected:
    StreamSocket^ m_Socket;
    HostName^     m_RemoteHostName;
    HANDLE        m_WaitEvent;
    NPT_Timeout   m_ReadTimeout;
    NPT_Timeout   m_WriteTimeout;
};

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::NPT_WinRtTcpClientSocket
+---------------------------------------------------------------------*/
NPT_WinRtTcpClientSocket::NPT_WinRtTcpClientSocket() :
    m_ReadTimeout(NPT_WINRT_SOCKET_DEFAULT_READ_TIMEOUT),
    m_WriteTimeout(NPT_WINRT_SOCKET_DEFAULT_WRITE_TIMEOUT)
{
    m_Socket = ref new StreamSocket();
    m_WaitEvent = CreateEventExW(NULL, L"", 0, EVENT_ALL_ACCESS);
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::NPT_WinRtTcpClientSocket
+---------------------------------------------------------------------*/
NPT_WinRtTcpClientSocket::~NPT_WinRtTcpClientSocket()
{
    CloseHandle(m_WaitEvent);
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::Bind
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtTcpClientSocket::Bind(const NPT_SocketAddress& address, bool reuse_address)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::Connect
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtTcpClientSocket::Connect(const NPT_SocketAddress& address, NPT_Timeout timeout)
{
    try {
        NPT_LOG_FINE_1("connecting to %s", address.GetIpAddress().m_HostName.GetChars());

        m_RemoteHostName = ref new HostName(StringFromUTF8(address.GetIpAddress().m_HostName.GetChars()));
        String^ remote_service = ref new String();
        NPT_String port = NPT_String::FromIntegerU(address.GetPort());
        IAsyncAction^ connection = m_Socket->ConnectAsync(m_RemoteHostName, StringFromUTF8(port.GetChars()));

        // wait for the connection to be established
        NPT_Result result = WaitForAsyncAction(connection, m_WaitEvent, timeout);
        if (NPT_FAILED(result)) {
            NPT_LOG_FINE_1("connection failed (%d)", result);
        } else {
            NPT_LOG_FINE("connected");
        }
        return result;
    } catch (Exception^ e) {
        NPT_LOG_FINE("exception caught");
        return NPT_FAILURE; 
    }
}

NPT_Result
    NPT_WinRtTcpClientSocket::WaitForConnection(NPT_Timeout timeout)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtTcpClientSocket::GetInputStream(NPT_InputStreamReference& stream)
{
    stream = new NPT_WinRtSocketInputStream(m_Socket, m_ReadTimeout);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::GetOutputStream
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtTcpClientSocket::GetOutputStream(NPT_OutputStreamReference& stream)
{
    stream = new NPT_WinRtSocketOutputStream(m_Socket, m_WriteTimeout);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::GetInfo
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtTcpClientSocket::GetInfo(NPT_SocketInfo& info)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::SetReadTimeout
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtTcpClientSocket::SetReadTimeout(NPT_Timeout timeout)
{
    m_ReadTimeout = timeout;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::SetWriteTimeout
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtTcpClientSocket::SetWriteTimeout(NPT_Timeout timeout)
{
    m_WriteTimeout = timeout;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtTcpClientSocket::Cancel
+---------------------------------------------------------------------*/
NPT_Result 
    NPT_WinRtTcpClientSocket::Cancel(bool shutdown)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_TcpClientSocket::NPT_TcpClientSocket
+---------------------------------------------------------------------*/
NPT_TcpClientSocket::NPT_TcpClientSocket(NPT_Flags flags) :
    NPT_Socket(NULL)
{
    m_SocketDelegate = new NPT_WinRtTcpClientSocket();
}

/*----------------------------------------------------------------------
|   NPT_TcpClientSocket::NPT_TcpClientSocket
+---------------------------------------------------------------------*/
NPT_TcpClientSocket::~NPT_TcpClientSocket()
{
    delete m_SocketDelegate;

    m_SocketDelegate = NULL;
}

/*----------------------------------------------------------------------
|   NPT_Socket::~NPT_Socket
+---------------------------------------------------------------------*/
NPT_Socket::~NPT_Socket()
{
    delete m_SocketDelegate;
}

/*----------------------------------------------------------------------
|   NPT_WinRtUdpSocket
+---------------------------------------------------------------------*/
class NPT_WinRtUdpSocket 
    : public NPT_SocketInterface
    , public NPT_UdpSocketInterface
{
public:
    // constructors and destructor
    NPT_WinRtUdpSocket(NPT_Flags flags);
    virtual ~NPT_WinRtUdpSocket();

    // NPT_SocketInterface methods
    virtual NPT_Result Bind(const NPT_SocketAddress& address, bool reuse_address = true);
    virtual NPT_Result Connect(const NPT_SocketAddress& address, NPT_Timeout timeout);
    virtual NPT_Result WaitForConnection(NPT_Timeout timeout);
    virtual NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    virtual NPT_Result GetOutputStream(NPT_OutputStreamReference& stream);
    virtual NPT_Result GetInfo(NPT_SocketInfo& info);
    virtual NPT_Result SetReadTimeout(NPT_Timeout timeout);
    virtual NPT_Result SetWriteTimeout(NPT_Timeout timeout);
    virtual NPT_Result Cancel(bool shutdown=true);

    // NPT_UdpSocketInterface methods
    virtual NPT_Result Send(const NPT_DataBuffer&    packet, 
        const NPT_SocketAddress* address = NULL);
    virtual NPT_Result Receive(NPT_DataBuffer&    packet, 
        NPT_SocketAddress* address = NULL);

protected:
    DatagramSocket^ m_Socket;
    IOutputStream^  m_OutputStream;
    DataWriter^     m_Writer;
    DatagramSocketMessageReceivedEventArgs^ m_Args;
    HANDLE          m_WaitEvent;
    HANDLE          m_WaitDataEvent;
    CRITICAL_SECTION m_Lock;
    NPT_Timeout     m_ReadTimeout;
    NPT_Timeout     m_WriteTimeout;
};

NPT_WinRtUdpSocket::NPT_WinRtUdpSocket(NPT_Flags flags)
    : m_ReadTimeout(NPT_WINRT_SOCKET_DEFAULT_READ_TIMEOUT)
    , m_WriteTimeout(NPT_WINRT_SOCKET_DEFAULT_WRITE_TIMEOUT)
{
    m_Socket = ref new DatagramSocket();
    m_WaitEvent = CreateEventExW(NULL, L"", 0, EVENT_ALL_ACCESS);
    m_WaitDataEvent = CreateEventExW(NULL, L"", 0, EVENT_ALL_ACCESS);
    InitializeCriticalSectionEx(&m_Lock, 0, 0);
    m_Socket->MessageReceived += ref new TypedEventHandler<DatagramSocket^, DatagramSocketMessageReceivedEventArgs^>([&](DatagramSocket^ socket, DatagramSocketMessageReceivedEventArgs^ args) {
        task<IOutputStream^>(socket->GetOutputStreamAsync(args->RemoteAddress, args->RemotePort)).then([this, socket, args] (IOutputStream^ stream) {
            EnterCriticalSection(&m_Lock);
            m_Args = args;
            if (m_OutputStream == nullptr) {
                m_OutputStream = stream;
                m_Writer = ref new DataWriter(m_OutputStream);
            }
            LeaveCriticalSection(&m_Lock);
        }).then([this, socket, args] (task<void> previousTask) {
            try {
                previousTask.get();
                SetEvent(m_WaitDataEvent);
            } catch (Exception^ exception) {
            }
        });
    });
}

NPT_WinRtUdpSocket::~NPT_WinRtUdpSocket()
{
    m_Writer->DetachStream();
    CloseHandle(m_WaitDataEvent);
    CloseHandle(m_WaitEvent);
}

NPT_Result NPT_WinRtUdpSocket::Bind(const NPT_SocketAddress& address, bool reuse_address/* = true*/)
{
    NPT_String port = NPT_String::FromIntegerU(address.GetPort());
    IAsyncAction^ bind_action;
    if (port.IsEmpty()) return NPT_ERROR_INVALID_PARAMETERS;
    if (address.GetIpAddress().m_HostName.IsEmpty()) {
        bind_action = m_Socket->BindServiceNameAsync(StringFromUTF8(port.GetChars()));
    } else {
        HostName^ hostname = ref new HostName(StringFromUTF8(address.GetIpAddress().m_HostName.GetChars()));
        bind_action = m_Socket->BindEndpointAsync(hostname, StringFromUTF8(port.GetChars()));
    }
    return WaitForAsyncAction(bind_action, m_WaitEvent);
}

NPT_Result NPT_WinRtUdpSocket::Connect(const NPT_SocketAddress& address, NPT_Timeout timeout)
{
    HostName^ remote_hostname = ref new HostName(StringFromUTF8(address.GetIpAddress().m_HostName.GetChars()));
    NPT_String port = NPT_String::FromIntegerU(address.GetPort());
    IAsyncAction^ bind_action = m_Socket->ConnectAsync(remote_hostname, StringFromUTF8(port.GetChars()));
    return WaitForAsyncAction(bind_action, m_WaitEvent);
}

NPT_Result NPT_WinRtUdpSocket::WaitForConnection(NPT_Timeout timeout)
{
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtUdpSocket::GetInputStream(NPT_InputStreamReference& stream)
{
    //stream = new NPT_WinRtSocketInputStream(m_Socket, m_ReadTimeout);
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtUdpSocket::GetOutputStream(NPT_OutputStreamReference& stream)
{
    //stream = new NPT_WinRtSocketOutputStream(m_Socket, m_WriteTimeout);
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtUdpSocket::GetInfo(NPT_SocketInfo& info)
{
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtUdpSocket::SetReadTimeout(NPT_Timeout timeout)
{
    m_ReadTimeout = timeout;
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtUdpSocket::SetWriteTimeout(NPT_Timeout timeout)
{
    m_WriteTimeout = timeout;
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtUdpSocket::Cancel(bool shutdown/*=true*/)
{
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtUdpSocket::Send(const NPT_DataBuffer&    packet, 
                                    const NPT_SocketAddress* address)
{
    NPT_Result result;
    if (address) {
        try {
            HostName^ remoteHostName = ref new HostName(StringFromUTF8(address->GetIpAddress().m_HostName.GetChars()));
            String^ remote_service = ref new String();
            NPT_String port = NPT_String::FromIntegerU(address->GetPort());
            IAsyncAction^ connection = m_Socket->ConnectAsync(remoteHostName, StringFromUTF8(port.GetChars()));

            result = WaitForAsyncAction(connection, m_WaitEvent);
            // wait for the connection to be established
            if (NPT_FAILED(result)) {
                NPT_LOG_FINE_1("connection failed (%d)", result);
                return result;
            } else {
                NPT_LOG_FINE("connected");
                Array<unsigned char>^ bytes = ref new Array<unsigned char>(packet.GetDataSize());
                NPT_CopyMemory(bytes->Data, packet.GetData(), packet.GetDataSize());
                if (m_Writer == nullptr) {
                    m_Writer = ref new DataWriter(m_Socket->OutputStream);
                }
                m_Writer->WriteBytes(bytes);
                auto operation = m_Writer->StoreAsync();
                unsigned int return_value = 0;
                result = WaitForAsyncOperation(operation, m_WaitEvent, return_value, m_WriteTimeout);
            }
        } catch (Exception^ e) {
            NPT_LOG_FINE("exception caught");
            return NPT_FAILURE; 
        }
    } else {
        Array<unsigned char>^ bytes = ref new Array<unsigned char>(packet.GetDataSize());
        NPT_CopyMemory(bytes->Data, packet.GetData(), packet.GetDataSize());
        m_Writer->WriteBytes(bytes);
        auto operation = m_Writer->StoreAsync();
        unsigned int return_value = 0;
        result = WaitForAsyncOperation(operation, m_WaitEvent, return_value, m_WriteTimeout);
    }
    return result;
}

NPT_Result NPT_WinRtUdpSocket::Receive(NPT_DataBuffer&    packet, 
                                       NPT_SocketAddress* address)
{
    NPT_Result result = NPT_SUCCESS;
    if (address && address->GetIpAddress().m_HostName.IsEmpty() == false) {
        try {
            HostName^ remoteHostName = ref new HostName(StringFromUTF8(address->GetIpAddress().m_HostName.GetChars()));
            String^ remote_service = ref new String();
            NPT_String port = NPT_String::FromIntegerU(address->GetPort());
            IAsyncAction^ connection = m_Socket->ConnectAsync(remoteHostName, StringFromUTF8(port.GetChars()));

            result = WaitForAsyncAction(connection, m_WaitEvent);
            // wait for the connection to be established
            if (NPT_FAILED(result)) {
                NPT_LOG_FINE_1("connection failed (%d)", result);
                return result;
            } else {
                ResetEvent(m_WaitDataEvent);
                DWORD wait_result = WaitForSingleObjectEx(m_WaitDataEvent, m_ReadTimeout, FALSE);
                if (wait_result != WAIT_OBJECT_0) {
                    NPT_LOG_FINE("receive data timed out");
                    return NPT_ERROR_TIMEOUT;
                }
                NPT_LOG_FINEST("done waiting for receive data");
                unsigned int bytes_available = m_Args->GetDataReader()->UnconsumedBufferLength;
                Array<unsigned char>^ bytes = ref new Array<unsigned char>(bytes_available);
                m_Args->GetDataReader()->ReadBytes(bytes);
                if (bytes_available <= packet.GetDataSize()) {
                    NPT_CopyMemory(packet.UseData(), bytes->Data, bytes_available);
                    packet.SetDataSize(bytes_available);
                } else {
                    NPT_CopyMemory(packet.UseData(), bytes->Data, packet.GetDataSize());
                    packet.SetDataSize(packet.GetDataSize());
                }
                //address->SetIpAddress(NPT_IpAddress(m_Args->RemoteAddress->RawName));
                //address->SetPort(m_Args->RemotePort);
                delete m_Args;
                return NPT_SUCCESS;
            }
        } catch (Exception^ e) {
            NPT_LOG_FINE("exception caught");
            return NPT_FAILURE; 
        }
    } else {
        try {
            ResetEvent(m_WaitDataEvent);
            DWORD wait_result = WaitForSingleObjectEx(m_WaitDataEvent, m_ReadTimeout, FALSE);
            if (wait_result != WAIT_OBJECT_0) {
                NPT_LOG_FINE("receive data timed out");
                return NPT_ERROR_TIMEOUT;
            }
            NPT_LOG_FINEST("done waiting for receive data");
            unsigned int bytes_available = m_Args->GetDataReader()->UnconsumedBufferLength;
            Array<unsigned char>^ bytes = ref new Array<unsigned char>(bytes_available);
            m_Args->GetDataReader()->ReadBytes(bytes);
            if (bytes_available <= packet.GetDataSize()) {
                NPT_CopyMemory(packet.UseData(), bytes->Data, bytes_available);
                packet.SetDataSize(bytes_available);
            } else {
                NPT_CopyMemory(packet.UseData(), bytes->Data, packet.GetDataSize());
                packet.SetDataSize(packet.GetDataSize());
            }
            return NPT_SUCCESS;
        } catch (Exception^ e) {
            NPT_LOG_FINE("exception caught");
            return NPT_FAILURE; 
        }
    }
    return result;
}

/*----------------------------------------------------------------------
|   NPT_UdpSocket::NPT_UdpSocket
+---------------------------------------------------------------------*/
NPT_UdpSocket::NPT_UdpSocket(NPT_Flags flags)
{
    NPT_WinRtUdpSocket * udp_socket_delegate = new NPT_WinRtUdpSocket(flags);
    m_SocketDelegate    = udp_socket_delegate;
    m_UdpSocketDelegate = udp_socket_delegate;
}

/*----------------------------------------------------------------------
|   NPT_UdpSocket::NPT_UdpSocket
+---------------------------------------------------------------------*/
NPT_UdpSocket::NPT_UdpSocket(NPT_UdpSocketInterface* delegate) :
    m_UdpSocketDelegate(delegate)
{
}

/*----------------------------------------------------------------------
|   NPT_UdpSocket::~NPT_UdpSocket
+---------------------------------------------------------------------*/
NPT_UdpSocket::~NPT_UdpSocket()
{
    delete m_UdpSocketDelegate;
    m_UdpSocketDelegate = NULL;
    m_SocketDelegate    = NULL;
}

/*----------------------------------------------------------------------
|   NPT_WinRtUdpMulticastSocket
+---------------------------------------------------------------------*/
class NPT_WinRtUdpMulticastSocket 
    : public NPT_UdpMulticastSocketInterface
    , protected NPT_WinRtUdpSocket
{
public:
    // methods
    NPT_WinRtUdpMulticastSocket(NPT_Flags flags);
    ~NPT_WinRtUdpMulticastSocket();

    // NPT_UdpMulticastSocketInterface methods
    NPT_Result JoinGroup(const NPT_IpAddress& group,
        const NPT_IpAddress& iface);
    NPT_Result LeaveGroup(const NPT_IpAddress& group,
        const NPT_IpAddress& iface);
    NPT_Result SetTimeToLive(unsigned char ttl);
    NPT_Result SetInterface(const NPT_IpAddress& iface);

    // friends 
    friend class NPT_UdpMulticastSocket;
};

/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::NPT_BsdUdpMulticastSocket
+---------------------------------------------------------------------*/
NPT_WinRtUdpMulticastSocket::NPT_WinRtUdpMulticastSocket(NPT_Flags flags) :
    NPT_WinRtUdpSocket(flags)
{
}

/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::~NPT_BsdUdpMulticastSocket
+---------------------------------------------------------------------*/
NPT_WinRtUdpMulticastSocket::~NPT_WinRtUdpMulticastSocket()
{
}

/*----------------------------------------------------------------------
|   NPT_WinRtUdpMulticastSocket::JoinGroup
+---------------------------------------------------------------------*/
NPT_Result
    NPT_WinRtUdpMulticastSocket::JoinGroup(const NPT_IpAddress& group,
    const NPT_IpAddress& iface)
{
    HostName^ group_addr = ref new HostName(StringFromUTF8(group.m_HostName.GetChars()));
    m_Socket->JoinMulticastGroup(group_addr);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtUdpMulticastSocket::LeaveGroup
+---------------------------------------------------------------------*/
NPT_Result
    NPT_WinRtUdpMulticastSocket::LeaveGroup(const NPT_IpAddress& group,
    const NPT_IpAddress& iface)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtUdpMulticastSocket::SetInterface
+---------------------------------------------------------------------*/
NPT_Result
    NPT_WinRtUdpMulticastSocket::SetInterface(const NPT_IpAddress& iface)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_WinRtUdpMulticastSocket::SetTimeToLive
+---------------------------------------------------------------------*/
NPT_Result
    NPT_WinRtUdpMulticastSocket::SetTimeToLive(unsigned char ttl)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_UdpMulticastSocket::NPT_UdpMulticastSocket
+---------------------------------------------------------------------*/
NPT_UdpMulticastSocket::NPT_UdpMulticastSocket(NPT_Flags flags) :
    NPT_UdpSocket((NPT_UdpSocketInterface*)0)
{
    NPT_WinRtUdpMulticastSocket * delegate = new NPT_WinRtUdpMulticastSocket(flags);
    m_SocketDelegate             = delegate;
    m_UdpSocketDelegate          = delegate;
    m_UdpMulticastSocketDelegate = delegate;
}

/*----------------------------------------------------------------------
|   NPT_UdpMulticastSocket::~NPT_UdpMulticastSocket
+---------------------------------------------------------------------*/
NPT_UdpMulticastSocket::~NPT_UdpMulticastSocket()
{
    delete m_SocketDelegate;
    m_SocketDelegate             = NULL;
    m_UdpSocketDelegate          = NULL;
    m_UdpMulticastSocketDelegate = NULL;
}

class NPT_WinRtTcpServerSocket
    : public NPT_TcpServerSocketInterface
    , public NPT_SocketInterface
{
public:
    NPT_WinRtTcpServerSocket(NPT_Flags flags);
    ~NPT_WinRtTcpServerSocket();

    // NPT_SocketInterface interface methods
    virtual NPT_Result Bind(const NPT_SocketAddress& address, bool reuse_address = true);
    virtual NPT_Result Connect(const NPT_SocketAddress& address, NPT_Timeout timeout);
    virtual NPT_Result WaitForConnection(NPT_Timeout timeout);
    virtual NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    virtual NPT_Result GetOutputStream(NPT_OutputStreamReference& stream);
    virtual NPT_Result GetInfo(NPT_SocketInfo& info);
    virtual NPT_Result SetReadTimeout(NPT_Timeout timeout);
    virtual NPT_Result SetWriteTimeout(NPT_Timeout timeout);
    virtual NPT_Result Cancel(bool shutdown=true);

    // NPT_TcpServerSocketInterface interface methods
    virtual NPT_Result Listen(unsigned int max_clients);
    virtual NPT_Result WaitForNewClient(NPT_Socket*& client, 
        NPT_Timeout  timeout,
        NPT_Flags    flags);
private:
    StreamSocketListener^       m_ServerSocketListener;
    unsigned int                m_ListenMax;
    HANDLE                      m_WaitEvent;
    NPT_Timeout                 m_ReadTimeout;
    NPT_Timeout                 m_WriteTimeout;
    StreamSocket^               m_ClientSocket;
    friend class NPT_TcpServerSocket;
};

NPT_WinRtTcpServerSocket::NPT_WinRtTcpServerSocket(NPT_Flags flags)
    : m_ReadTimeout(NPT_WINRT_SOCKET_DEFAULT_READ_TIMEOUT)
    , m_WriteTimeout(NPT_WINRT_SOCKET_DEFAULT_WRITE_TIMEOUT)
    , m_ListenMax(0)
{
    m_ServerSocketListener = ref new StreamSocketListener();
    m_WaitEvent = CreateEventExW(NULL, L"", 0, EVENT_ALL_ACCESS);
    m_ServerSocketListener->ConnectionReceived += ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs^>([&](StreamSocketListener^ listener, StreamSocketListenerConnectionReceivedEventArgs^ args) {
        m_ClientSocket = args->Socket;
        SetEvent(m_WaitEvent);
    });
}

NPT_WinRtTcpServerSocket::~NPT_WinRtTcpServerSocket()
{
    CloseHandle(m_WaitEvent);
}

NPT_Result NPT_WinRtTcpServerSocket::Bind(const NPT_SocketAddress& address, bool reuse_address/* = true*/)
{
    NPT_String port = NPT_String::FromIntegerU(address.GetPort());
    IAsyncAction^ bind_action;
    if (port.IsEmpty()) return NPT_ERROR_INVALID_PARAMETERS;
    if (address.GetIpAddress().m_HostName.IsEmpty()) {
        bind_action = m_ServerSocketListener->BindServiceNameAsync(StringFromUTF8(port.GetChars()));
    } else {
        HostName^ hostname = ref new HostName(StringFromUTF8(address.GetIpAddress().m_HostName.GetChars()));
        bind_action = m_ServerSocketListener->BindEndpointAsync(hostname, StringFromUTF8(port.GetChars()));
    }
    return WaitForAsyncAction(bind_action, m_WaitEvent);
}

NPT_Result NPT_WinRtTcpServerSocket::Connect(const NPT_SocketAddress& address, NPT_Timeout timeout)
{
    return NPT_ERROR_NOT_SUPPORTED;
}

NPT_Result NPT_WinRtTcpServerSocket::WaitForConnection(NPT_Timeout timeout)
{
    return NPT_ERROR_NOT_SUPPORTED;
}

NPT_Result NPT_WinRtTcpServerSocket::GetInputStream(NPT_InputStreamReference& stream)
{
    // no stream
    stream = NULL;
    return NPT_ERROR_NOT_SUPPORTED;
}

NPT_Result NPT_WinRtTcpServerSocket::GetOutputStream(NPT_OutputStreamReference& stream)
{
    // no stream
    stream = NULL;
    return NPT_ERROR_NOT_SUPPORTED;
}

NPT_Result NPT_WinRtTcpServerSocket::GetInfo(NPT_SocketInfo& info)
{
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtTcpServerSocket::SetReadTimeout(NPT_Timeout timeout)
{
    m_ReadTimeout = timeout;
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtTcpServerSocket::SetWriteTimeout(NPT_Timeout timeout)
{
    m_WriteTimeout = timeout;
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtTcpServerSocket::Cancel(bool shutdown/*=true*/)
{
    if (shutdown) {
        //m_ServerSocketListener->Close();
    }
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtTcpServerSocket::Listen(unsigned int max_clients)
{
    m_ListenMax = max_clients;
    return NPT_SUCCESS;
}

NPT_Result NPT_WinRtTcpServerSocket::WaitForNewClient(NPT_Socket*& client, 
                                    NPT_Timeout  timeout,
                                    NPT_Flags    flags)
{
    NPT_Result result = NPT_SUCCESS;
    client = NULL;
    ResetEvent(m_WaitEvent);
    DWORD wait_result = WaitForSingleObjectEx(m_WaitEvent, timeout, FALSE);
    if (wait_result != WAIT_OBJECT_0) {
        NPT_LOG_FINE("wait timed out, canceling...");
        // cancel now
        WaitForSingleObjectEx(m_WaitEvent, INFINITE, FALSE);
    }
    client = (NPT_Socket *)(new NPT_WinRtTcpSocket(m_ClientSocket, flags));
    return result;
}

/*----------------------------------------------------------------------
|   NPT_TcpServerSocket::NPT_TcpServerSocket
+---------------------------------------------------------------------*/
NPT_TcpServerSocket::NPT_TcpServerSocket(NPT_Flags flags)
{
    NPT_WinRtTcpServerSocket * delegate = new NPT_WinRtTcpServerSocket(flags);

    m_SocketDelegate          = delegate;
    m_TcpServerSocketDelegate = delegate;
}

/*----------------------------------------------------------------------
|   NPT_TcpServerSocket::NPT_TcpServerSocket
+---------------------------------------------------------------------*/
NPT_TcpServerSocket::~NPT_TcpServerSocket()
{
    delete m_SocketDelegate;
    m_SocketDelegate          = NULL;
    m_TcpServerSocketDelegate = NULL;
}
