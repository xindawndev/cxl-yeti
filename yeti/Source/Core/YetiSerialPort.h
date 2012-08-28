#ifndef _CXL_YETI_SERIAL_PORT_H_
#define _CXL_YETI_SERIAL_PORT_H_

#include "YetiTypes.h"
#include "YetiStreams.h"

const int YETI_ERROR_NO_SUCH_SERIAL_PORT      = YETI_ERROR_BASE_SERIAL_PORT - 0;
const int YETI_ERROR_SERIAL_PORT_NOT_OPEN     = YETI_ERROR_BASE_SERIAL_PORT - 1;
const int YETI_ERROR_SERIAL_PORT_ALREADY_OPEN = YETI_ERROR_BASE_SERIAL_PORT - 2;
const int YETI_ERROR_SERIAL_PORT_BUSY         = YETI_ERROR_BASE_SERIAL_PORT - 3;

NAMEBEG

typedef enum {
    YETI_SERIAL_PORT_PARITY_NONE,
    YETI_SERIAL_PORT_PARITY_EVEN,
    YETI_SERIAL_PORT_PARITY_ODD,
    YETI_SERIAL_PORT_PARITY_MARK
} SerialPortParity;

typedef enum {
    YETI_SERIAL_PORT_STOP_BITS_1,
    YETI_SERIAL_PORT_STOP_BITS_1_5,
    YETI_SERIAL_PORT_STOP_BITS_2
} SerialPortStopBits;

typedef enum {
    YETI_SERIAL_PORT_FLOW_CONTROL_NONE,
    YETI_SERIAL_PORT_FLOW_CONTROL_HARDWARE,
    YETI_SERIAL_PORT_FLOW_CONTROL_XON_XOFF
} SerialPortFlowControl;

class SerialPortInterface
{
public:
    virtual ~SerialPortInterface() {}

    virtual YETI_Result open(unsigned int speed,
        SerialPortStopBits stop_bits,
        SerialPortFlowControl flow_control,
        SerialPortParity parity) = 0;
    virtual YETI_Result close() = 0;
    virtual YETI_Result get_inputstream(InputStreamReference & stream) = 0;
    virtual YETI_Result get_outputstream(OutputStreamReference & stream) = 0;
};

class SerialPort : public SerialPortInterface
{
public:
    SerialPort(const char * name);
    ~SerialPort() { delete m_delegate_; }

    YETI_Result open(unsigned int speed,
        SerialPortStopBits stop_bits,
        SerialPortFlowControl flow_control,
        SerialPortParity parity) {
            return m_delegate_->open(speed, stop_bits, flow_control, parity);
    }

    YETI_Result close() {
        return m_delegate_->close();
    }

    YETI_Result get_inputstream(InputStreamReference & stream) {
        return m_delegate_->get_inputstream(stream);
    }

    YETI_Result get_outputstream(OutputStreamReference & stream) {
        return m_delegate_->get_outputstream(stream);
    }

protected:
    SerialPortInterface * m_delegate_;
};

NAMEEND

#endif // _CXL_YETI_LIST_H_
