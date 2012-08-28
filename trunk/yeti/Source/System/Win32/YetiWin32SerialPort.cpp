#include <windows.h>

#include "YetiUtil.h"
#include "YetiSerialPort.h"
#include "YetiString.h"
#include "YetiLogging.h"

NAMEBEG

class Win32HandleWrapper
{
public:
    Win32HandleWrapper(HANDLE handle)
        : m_handle_(handle) {}
    ~Win32HandleWrapper() {
        ::CloseHandle(m_handle_);
    }

    HANDLE get_handle() { return m_handle_; }

private:
    HANDLE m_handle_;
};

typedef Reference<Win32HandleWrapper> Win32HandleReference;

class Win32SerialPortStream
{
public:
    Win32SerialPortStream(Win32HandleReference handle)
        : m_handlereference_(handle) {}

protected:
    virtual ~Win32SerialPortStream() {}
    Win32HandleReference m_handlereference_;
};

class Win32SerialPortInputStream
    : public InputStream
    , Win32SerialPortStream
{
public:
    Win32SerialPortInputStream(Win32HandleReference & handle)
        : Win32SerialPortStream(handle) {}

    YETI_Result read(void * buffer,
        YETI_Size bytes_to_read,
        YETI_Size * bytes_read) {
            DWORD nb_read = 0;
            BOOL result = ::ReadFile(m_handlereference_->get_handle(),
                buffer, bytes_to_read, &nb_read, NULL);

            if (result == TRUE) {
                if (bytes_read) *bytes_read = nb_read;
                return YETI_SUCCESS;
            } else {
                if (bytes_read) *bytes_read = 0;
                return YETI_FAILURE;
            }
    }

    YETI_Result seek(YETI_Position offset) {
        return YETI_ERROR_NOT_SUPPORTED;
    }

    YETI_Result tell(YETI_Position & offset) {
        return YETI_ERROR_NOT_SUPPORTED;
    }

    YETI_Result get_size(YETI_LargeSize & size) {
        return YETI_ERROR_NOT_SUPPORTED;
    }

    YETI_Result get_available(YETI_LargeSize & available) {
        return YETI_ERROR_NOT_SUPPORTED;
    }
};

class Win32SerialPortOutputStream
    : public OutputStream
    , public Win32SerialPortStream
{
public:
    Win32SerialPortOutputStream(Win32HandleReference & handle)
        : Win32SerialPortStream(handle) {}

    YETI_Result write(const void * buffer,
        YETI_Size bytes_to_write,
        YETI_Size * bytes_written) {
        DWORD nb_written = 0;
        BOOL result = ::WriteFile(m_handlereference_->get_handle(),
            buffer, bytes_to_write, &nb_written, NULL);

        if (result == TRUE) {
            if (bytes_written) *bytes_written = nb_written;
            return YETI_SUCCESS;
        } else {
            if (bytes_written) *bytes_written = nb_written;
            return YETI_FAILURE;
        }
    }

    YETI_Result seek(YETI_Position offset) {
        return YETI_ERROR_NOT_SUPPORTED;
    }

    YETI_Result tell(YETI_Position & offset) {
        return YETI_ERROR_NOT_SUPPORTED;
    }
};

class Win32SerialPort
    : public SerialPortInterface
{
public:
    Win32SerialPort(const char * name)
        : m_name_(name) {}
    ~Win32SerialPort() { close(); }

    YETI_Result open(unsigned int speed,
        SerialPortStopBits stop_bits        = YETI_SERIAL_PORT_STOP_BITS_1,
        SerialPortFlowControl flow_control  = YETI_SERIAL_PORT_FLOW_CONTROL_NONE,
        SerialPortParity parity             = YETI_SERIAL_PORT_PARITY_NONE);

    YETI_Result close() {
        m_handle_reference_ = NULL;
        return YETI_SUCCESS;
    }

    YETI_Result get_inputstream(InputStreamReference & stream) {
        stream = NULL;
        if (m_handle_reference_.is_null()) return YETI_ERROR_SERIAL_PORT_NOT_OPEN;
        stream = new Win32SerialPortInputStream(m_handle_reference_);
        return YETI_SUCCESS;
    }

    YETI_Result get_outputstream(OutputStreamReference & stream) {
        stream = NULL;
        if (m_handle_reference_.is_null()) return YETI_ERROR_SERIAL_PORT_NOT_OPEN;
        stream = new Win32SerialPortOutputStream(m_handle_reference_);
        return YETI_SUCCESS;
    }

private:
    String               m_name_;
    Win32HandleReference m_handle_reference_;
};

YETI_Result Win32SerialPort::open(unsigned int speed,
                 SerialPortStopBits stop_bits        /*= YETI_SERIAL_PORT_STOP_BITS_1*/,
                 SerialPortFlowControl flow_control  /*= YETI_SERIAL_PORT_FLOW_CONTROL_NONE*/,
                 SerialPortParity parity             /*= YETI_SERIAL_PORT_PARITY_NONE*/)
{
    if (!m_handle_reference_.is_null()) {
        return YETI_ERROR_SERIAL_PORT_ALREADY_OPEN;
    }

    HANDLE handle = ::CreateFile(m_name_,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        0,
        0);
    if (handle == INVALID_HANDLE_VALUE) {
        return YETI_ERROR_NO_SUCH_SERIAL_PORT;
    }

    DCB dcb;
    SetMemory(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(DCB);
    if (!::GetCommState(handle, &dcb)) {
        ::CloseHandle(handle);
        return YETI_FAILURE;
    }
    dcb.fBinary = TRUE;
    dcb.BaudRate = speed;
    switch (stop_bits) {
case YETI_SERIAL_PORT_STOP_BITS_1: dcb.StopBits = ONESTOPBIT; break;
case YETI_SERIAL_PORT_STOP_BITS_1_5: dcb.StopBits = ONE5STOPBITS; break;
case YETI_SERIAL_PORT_STOP_BITS_2: dcb.StopBits = TWOSTOPBITS; break;
    }
    switch (flow_control) {
case YETI_SERIAL_PORT_FLOW_CONTROL_NONE:
    dcb.fOutX = dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = FALSE;
    dcb.fInX = dcb.fDsrSensitivity = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    break;

case YETI_SERIAL_PORT_FLOW_CONTROL_HARDWARE:
    dcb.fOutX = dcb.fOutxDsrFlow = FALSE;
    dcb.fOutxCtsFlow = TRUE;
    dcb.fInX = dcb.fDsrSensitivity = FALSE;
    dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    break;

case YETI_SERIAL_PORT_FLOW_CONTROL_XON_XOFF:
    dcb.fOutX = TRUE;
    dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = FALSE;
    dcb.fInX = TRUE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    break;
    }
    switch (parity) {
case YETI_SERIAL_PORT_PARITY_NONE: dcb.fParity = FALSE; dcb.Parity = NOPARITY; break;
case YETI_SERIAL_PORT_PARITY_EVEN: dcb.fParity = TRUE;  dcb.Parity = EVENPARITY; break;
case YETI_SERIAL_PORT_PARITY_ODD: dcb.fParity  = TRUE;  dcb.Parity = ODDPARITY; break;
case YETI_SERIAL_PORT_PARITY_MARK: dcb.fParity = TRUE;  dcb.Parity = MARKPARITY; break;
    }

    if (!::SetCommState(handle, &dcb)) {
        ::CloseHandle(handle);
        return YETI_FAILURE;
    }

    m_handle_reference_ = new Win32HandleWrapper(handle);

    return YETI_SUCCESS;
}


SerialPort::SerialPort(const char * name)
{
    m_delegate_ = new Win32SerialPort(name);
}

NAMEEND
