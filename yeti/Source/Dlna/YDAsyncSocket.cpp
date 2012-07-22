#include "YDAsyncSocket.h"

namespace yeti
{
    namespace dlna
    {
        AsyncSocket::AsyncSocket(const Engine & engine)
            : m_engine_(engine)
        {
            Engine::get_singleton().reg_object(this);
        }

        void AsyncSocket::set_reallocate_notification_callback(OnBufferReAllocated callback)
        {

        }

        void * AsyncSocket::get_user()
        {
            return NULL;
        }

        //ILibAsyncSocket_SocketModule ILibCreateAsyncSocketModule(void *Chain, int initialBufferSize, ILibAsyncSocket_OnData , ILibAsyncSocket_OnConnect OnConnect ,ILibAsyncSocket_OnDisconnect OnDisconnect,ILibAsyncSocket_OnSendOK OnSendOK);

        void * AsyncSocket::get_socket()
        {
            return NULL;
        }

        unsigned int AsyncSocket::get_pending_bytes_to_send()
        {
            return 0;
        }

        unsigned int AsyncSocket::get_total_bytes_sent()
        {
            return 0;
        }

        void AsyncSocket::reset_total_bytes_sent()
        {

        }

        void AsyncSocket::connect_to(int localInterface, int remoteInterface, int remote_port_number, OnInterrupt interrupt_ptr, void * user)
        {

        }

        enum AsyncSocket::TypeSendStatus AsyncSocket::send_to(char * buffer, int length, int remote_address, unsigned short remote_port, TypeMOS user_free)
        {
            return AsyncSocket::STATUS_ALL_DATA_SENT;
        };

        void AsyncSocket::disconnect()
        {

        }

        void AsyncSocket::get_buffer(char ** buffer, int * begin_pointer, int * endpointer)
        {

        }

        void AsyncSocket::use_this_socket(void * the_socket, OnInterrupt interrupt_ptr, void * user)
        {

        }

        void AsyncSocket::set_remote_address(int remote_address)
        {

        }

        void AsyncSocket::set_local_interface2(int local_interface2)
        {

        }

        int AsyncSocket::is_free()
        {
            return 0;
        }

        int AsyncSocket::get_local_interface()
        {
            return 0;
        }

        unsigned short AsyncSocket::get_local_port()
        {
            return 0;
        }

        int AsyncSocket::get_remote_interface()
        {
            return 0;
        }

        unsigned short AsyncSocket::get_remote_port()
        {
            return 0;
        }

        void AsyncSocket::resume()
        {

        }

        int AsyncSocket::was_closed_because_buffer_size_exceeded()
        {
            return 0;
        }

        void AsyncSocket::set_maximum_buffer_size(int max_size, OnBufferSizeExceeded on_buffer_size_exceeded_callback, void * user)
        {

        }

        int AsyncSocket::initialize_QOS()
        {
            return 0;
        }

        void AsyncSocket::set_QOS_priority(TypeQOSPriority priority)
        {

        }

        void AsyncSocket::unitialize_QOS()
        {

        }

        void AsyncSocket::pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime)
        {

        }

        void AsyncSocket::post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset)
        {

        }

        void AsyncSocket::destroy(IObject * object)
        {

        }
    }
}
