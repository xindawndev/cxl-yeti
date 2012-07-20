#ifndef _YETI_DLNA_YDASYNCSOCKET_H_
#define _YETI_DLNA_YDASYNCSOCKET_H_

#include "YDCommon.h"
#include "YDEngine.h"

namespace yeti
{
    namespace dlna
    {

        class AsyncSocket : public IObject
        {
        public:
            AsyncSocket(const Engine & engine);
            virtual ~AsyncSocket() {}

        public:
            const Engine & engine() const {
                return m_engine_;
            }

        public:
            YETI_Timeout m_connect_timeout_;
            YETI_Size m_memory_chunk_size_;
            typedef enum {
                STATUS_ALL_DATA_SENT                = 0,
                STATUS_NOT_ALL_DATA_SENT_YET        = 1,
                STATUS_SEND_ON_CLOSED_SOCKET_ERROR  = -4
            } TypeSendStatus;

            typedef enum {
                QOS_NONE                = 0,
                QOS_BEST_EFFORT         = 1,
                QOS_BACKGROUND          = 2,
                QOS_EXCELLENT_EFFORT    = 3,
                QOS_AUDIO_VIDEO         = 4,
                QOS_VOICE               = 5,
                QOS_CONTROL             = 6
            } TypeQOSPriority;

            typedef enum {
                MOS_ENGINE  = 0,
                MOS_STATIC  = 1,
                MOS_USER    = 2
            } TypeMOS;

            typedef void(* OnReplaceSocket)(AsyncSocket * socket_mode, void * user);
            typedef void(* OnBufferSizeExceeded)(AsyncSocket * socket_mode, void * user);
            typedef void(* OnInterrupt)(AsyncSocket * socket_mode, void * user);
            typedef void(* OnData)(AsyncSocket * socket_mode,char * buffer,int * p_begin_pointer, int end_pointer, OnInterrupt * on_interrupt, void ** user, int * pause);
            typedef void(* OnConnect)(AsyncSocket * socket_mode, int connected, void * user);
            typedef void(* OnDisconnect)(AsyncSocket * socket_mode, void * user);
            typedef void(* OnSendOK)(AsyncSocket * socket_mode, void * user);
            typedef void(* OnBufferReAllocated)(AsyncSocket * socket_mode, void * user, ptrdiff_t new_offset);

        public:
            void set_reallocate_notification_callback(OnBufferReAllocated callback);
            void * get_user();

            //ILibAsyncSocket_SocketModule ILibCreateAsyncSocketModule(void *Chain, int initialBufferSize, ILibAsyncSocket_OnData , ILibAsyncSocket_OnConnect OnConnect ,ILibAsyncSocket_OnDisconnect OnDisconnect,ILibAsyncSocket_OnSendOK OnSendOK);

            void* get_socket();

            unsigned int get_pending_bytes_to_send();
            unsigned int get_total_bytes_sent();
            void reset_total_bytes_sent();

            void connect_to(int localInterface, int remoteInterface, int remote_port_number, OnInterrupt interrupt_ptr, void * user);
            enum TypeSendStatus send_to(char * buffer, int length, int remote_address, unsigned short remote_port, TypeMOS user_free);
            TypeSendStatus send(char * buffer, int length, TypeMOS user_free) {
                send_to(buffer, length, 0, 0, user_free);
            }
            void disconnect();
            void get_buffer(char ** buffer, int * begin_pointer, int * endpointer);
            void use_this_socket(void * the_socket, OnInterrupt interrupt_ptr, void * user);
            void set_remote_address(int remote_address);
            void set_local_interface2(int local_interface2);

            int is_free();
            int get_local_interface();
            unsigned short get_local_port();
            int get_remote_interface();
            unsigned short get_remote_port();

            void resume();
            int was_closed_because_buffer_size_exceeded();
            void set_maximum_buffer_size(int max_size, OnBufferSizeExceeded on_buffer_size_exceeded_callback, void * user);

            int initialize_QOS();
            void set_QOS_priority(TypeQOSPriority priority);
            void unitialize_QOS();

        public:
            virtual void pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime);
            virtual void post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset);
            virtual void destroy(IObject * object);

        private:
            typedef struct {
                char * buffer;
                int buffer_size;
                int bytes_sent;
                int remote_address;
                unsigned short remote_port;
                int user_free;
                struct SendData * next;
            } SendData;
        private:
            const Engine & m_engine_;
        };

    }
}

#endif // _YETI_DLNA_YDASYNCSOCKET_H_
