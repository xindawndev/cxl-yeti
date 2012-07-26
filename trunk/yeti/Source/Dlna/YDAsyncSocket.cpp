#include "YDAsyncSocket.h"

#include "YDTimer.h"

namespace yeti
{
    namespace dlna
    {
        AsyncSocket::AsyncSocket(const Engine & engine,
            int initial_buffer_size,
            AsyncSocket::OnData on_data,
            AsyncSocket::OnConnect on_connect,
            AsyncSocket::OnDisconnect on_disconnect,
           AsyncSocket::OnSendOK on_send_OK)
            : m_engine_(engine)
            , m_internal_socket_(-1)
            , m_on_data_(on_data)
            , m_on_connect_(on_connect)
            , m_on_disconnect_(on_disconnect)
            , m_on_send_OK_(on_send_OK)
            , m_buffer_(new char(initial_buffer_size))
            , m_initial_size_(initial_buffer_size)
            , m_malloc_size_(initial_buffer_size)
            , m_life_time_(new Timer(engine))
            , m_timeout_timer_(new Timer(engine))
            , m_replace_socket_timer_(new Timer(engine))
        {
            Engine::get_singleton().reg_object(this);
        }

        void AsyncSocket::set_reallocate_notification_callback(OnBufferReAllocated callback)
        {
            m_on_buffer_reallocated_ = callback;
        }

        void * AsyncSocket::get_user()
        {
            return m_user_;
        }

        void * AsyncSocket::get_socket()
        {
            return (void *)m_internal_socket_;
        }

        unsigned int AsyncSocket::get_pending_bytes_to_send()
        {
            return m_pending_bytes_to_send_;
        }

        unsigned int AsyncSocket::get_total_bytes_sent()
        {
            return m_total_bytes_sent_;
        }

        void AsyncSocket::reset_total_bytes_sent()
        {
            m_total_bytes_sent_ = 0;
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
#if defined(_WIN32_WCE) || defined(WIN32)
            SOCKET s;
#elif defined(_POSIX)
            int s;
#endif
            //if (!ILibIsChainBeingDestroyed(module->Chain)) {
            //    ILibLifeTime_Remove(module->TimeoutTimer,module);
            //    m_timeout_timer_->remove();
            //}

            m_send_lock_.lock();

            if (m_internal_socket_ != ~0) {
                m_pause_ = 1;
                s = m_internal_socket_;
                if (m_current_QOS_priorty_ != QOS_NONE) {
                    unitialize_QOS();
                }
                m_internal_socket_ = ~0;
                if (s != -1) {
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
                    shutdown(s, SD_BOTH);
#endif
                    closesocket(s);
#elif defined(_POSIX)
                    shutdown(s,SHUT_RDWR);
                    close(s);
#endif
                }

                //ILibAsyncSocket_ClearPendingSend(socketModule);
                m_send_lock_.unlock();
                if (m_on_disconnect_ != NULL) {
                    m_on_disconnect_(this, m_user_);
                }
            } else {
                m_send_lock_.unlock();
            }
        }

        void AsyncSocket::get_buffer(char ** buffer, int * begin_pointer, int * endpointer)
        {
            *buffer = m_buffer_;
            *begin_pointer = m_begin_pointer_;
            *endpointer = m_end_pointer_;
        }

        void AsyncSocket::use_this_socket(const AsyncSocket & other, void * use_this_socket, OnInterrupt interrupt_ptr, void * user)
        {
#if defined(_WIN32_WCE) || defined(WIN32)
            SOCKET the_socket = *((SOCKET*)use_this_socket);
#elif defined(_POSIX)
            int the_socket = *((int*)use_this_socket);
#endif
            int flags;

            m_pending_bytes_to_send_ = 0;
            m_total_bytes_sent_ = 0;
            m_internal_socket_ = the_socket;
            m_on_interrupt_ = interrupt_ptr;
            m_user_ = user;
            m_fin_connect_ = 1;
            m_pause_ = 0;

            m_buffer_ = (char*)realloc(other.m_buffer_, other.m_initial_size_);
            m_malloc_size_ = other.m_initial_size_;
            m_fin_connect_ = 1;
            m_begin_pointer_ = 0;
            m_end_pointer_ = 0;

#if defined(_WIN32_WCE) || defined(WIN32)
            flags = 1;
            ioctlsocket(other.m_internal_socket_, FIONBIO, (u_long *)&flags);
#elif defined(_POSIX)
            flags = fcntl(other.m_internal_socket_, F_GETFL, 0);
            fcntl(other.m_internal_socket_, F_SETFL, O_NONBLOCK | flags);
#endif
        }

        void AsyncSocket::set_remote_address(int remote_address)
        {
            m_remote_IP_address_ = remote_address;
        }

        void AsyncSocket::set_local_interface2(int local_interface2)
        {
            m_local_IP_address2_ = local_interface2;
        }

        int AsyncSocket::is_free()
        {
            return m_internal_socket_ == ~0 ? 1 : 0;
        }

        int AsyncSocket::get_local_interface()
        {
            if (m_local_IP_address2_ != 0) {
                return m_local_IP_address2_;
            }

            struct sockaddr_in receiving_address;
            int receiving_addr_length = sizeof(struct sockaddr_in);

            getsockname(m_internal_socket_, (struct sockaddr *)&receiving_address, &receiving_addr_length);
            return (receiving_address.sin_addr.s_addr);
        }

        unsigned short AsyncSocket::get_local_port()
        {
            struct sockaddr_in receiving_address;
            int receiving_addr_length = sizeof(struct sockaddr_in);
            ::getsockname(m_internal_socket_, (struct sockaddr *)&receiving_address, &receiving_addr_length);
            return ::ntohs(receiving_address.sin_port);
        }

        int AsyncSocket::get_remote_interface()
        {
            return m_remote_IP_address_;
        }

        unsigned short AsyncSocket::get_remote_port()
        {
            return m_remote_port_;
        }

        void AsyncSocket::resume()
        {
            m_pause_ = -1;
            Engine::get_singleton().force_unblock();
        }

        int AsyncSocket::was_closed_because_buffer_size_exceeded()
        {
            return m_max_buffer_size_exceeded_;
        }

        void AsyncSocket::set_maximum_buffer_size(int max_size, OnBufferSizeExceeded on_buffer_size_exceeded_callback, void * user)
        {
            m_max_buffer_size_ = max_size;
            m_on_buffer_size_exceeded_ = on_buffer_size_exceeded_callback;
            m_max_buffer_size_user_object_ = user;
        }

        int AsyncSocket::initialize_QOS()
        {
#if defined(WIN32) && defined(WIN32_QWAVE)
            QOS_VERSION version;
            version.MajorVersion = 1;
            version.MinorVersion = 0;
#endif
            if (m_QOS_initialized_ == 0) {
#if defined(WIN32) && defined(WIN32_QWAVE)
                m_QOS_initialized_ = !QOSCreateHandle(&version, &(m_QOS_handle_));
#endif
            }
            return m_QOS_initialized_;
        }

        void AsyncSocket::set_QOS_priority(TypeQOSPriority priority)
        {
#if defined(WIN32) && defined(WIN32_QWAVE)
            QOS_TRAFFIC_TYPE qtt;
            BOOL r;
            DWORD last_err;
#endif
            switch (priority)
            {
            case QOS_BACKGROUND:
#if defined(WIN32) && defined(WIN32_QWAVE)
                qtt = QOSTrafficTypeBackground;
#endif
                break;
            case QOS_EXCELLENT_EFFORT:
#if defined(WIN32) && defined(WIN32_QWAVE)
                qtt = QOSTrafficTypeExcellentEffort;
#endif
                break;
            case QOS_AUDIO_VIDEO:
#if defined(WIN32) && defined(WIN32_QWAVE)
                qtt = QOSTrafficTypeAudioVideo;
#endif
                break;
            case QOS_VOICE:
#if defined(WIN32) && defined(WIN32_QWAVE)
                qtt = QOSTrafficTypeVoice;
#endif            
                break;
            case QOS_CONTROL:
#if defined(WIN32) && defined(WIN32_QWAVE)
                qtt = QOSTrafficTypeControl;
#endif    
                break;
            default:
#if defined(WIN32) && defined(WIN32_QWAVE)
                qtt = QOSTrafficTypeBestEffort;
#endif    
                break;
            }

            if (m_current_QOS_priorty_ != priority) {
#if defined(WIN32) && defined(WIN32_QWAVE)
                if (m_QOS_flow_id_ == 0) {
                    r = QOSAddSocketToFlow(m_QOS_handle_, m_internal_socket_, NULL, qtt, QOS_NON_ADAPTIVE_FLOW, &(m_QOS_flow_id_));
                } else {
                    r = QOSSetFlow(m_QOS_handle_, m_QOS_flow_id_, QOSSetTrafficType, sizeof(QOS_TRAFFIC_TYPE), &qtt, 0, NULL);
                    if (r == 0 && GetLastError() == ERROR_NOT_FOUND) {
                        m_QOS_flow_id_ = 0;
                        r = QOSAddSocketToFlow(m_QOS_handle_, m_internal_socket_, NULL, qtt, QOS_NON_ADAPTIVE_FLOW, &(m_QOS_flow_id_));
                    }
                }
                if (r == 0) {
                    last_err = GetLastError();
                    if (last_err == ERROR_NOT_SUPPORTED) {
                        printf("QOS not supported!\r\n");
                    }
                } else {
                    printf("Successfully set QOS priority\r\n");
                }
#endif
                m_current_QOS_priorty_ = priority;
            }
        }

        void AsyncSocket::unitialize_QOS()
        {
#if defined(WIN32) && defined(WIN32_QWAVE)
            BOOL r;
            m_current_QOS_priorty_ = QOS_NONE;
            r = QOSRemoveSocketFromFlow(m_QOS_handle_, 0, m_QOS_flow_id_, 0);
            r = QOSCloseHandle(m_QOS_handle_);
            m_QOS_handle_ = 0;
            m_QOS_flow_id_ = 0;
#endif
            m_QOS_initialized_ = 0;
        }

        void AsyncSocket::pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime)
        {
            m_send_lock_.lock();
            if (m_internal_socket_ != -1) {
                if (m_pause_ < 0) {
                    *blocktime = 0;
                }
                if (m_fin_connect_ == 0) {
                    FD_SET(m_internal_socket_, (fd_set *)writeset);
                    FD_SET(m_internal_socket_, (fd_set *)errorset);
                } else {
                    if (m_pause_ == 0) {
                        FD_SET(m_internal_socket_, (fd_set *)readset);
                        FD_SET(m_internal_socket_, (fd_set *)errorset);
                    }
                }
            }
            if (m_pending_bytes_to_send_ != NULL) {
                FD_SET(m_internal_socket_, (fd_set *)writeset);
            }
            m_send_lock_.unlock();
        }

        void AsyncSocket::post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset)
        {
            int trigger_send_ok = 0;
            AsyncSocket::SendData * temp;
            int bytes_sent = 0;
            int flags;
            struct sockaddr_in receiving_address_;
            int receiving_addr_len = sizeof(struct sockaddr_in);
            int try_to_send = 1;

            int trigger_read_set = 0;
            int trigger_resume = 0;
            int trigger_write_set = 0;
            int trigger_error_set = 0;

            struct sockaddr_in dest;
            int destlen = sizeof(struct sockaddr_in);

            m_send_lock_.lock();
            if (m_fin_connect_ != 0 && m_internal_socket_ != ~0
                && FD_ISSET(m_internal_socket_, (fd_set *)writeset) != 0) {
                // send data
                while (try_to_send != 0) {
                    if (m_pending_send_head_->remote_address == 0
                        && m_pending_send_head_->remote_port == 0) {
#if defined(MSG_NOSIGNAL)
                        bytes_sent = ::send(m_internal_socket_,
                            m_pending_send_head_->buffer + m_pending_send_head_->bytes_sent,
                            m_pending_send_head_->buffer_size -  m_pending_send_head_->bytes_sent,
                            MSG_NOSIGNAL);
#elif defined(WIN32)
                        bytes_sent = ::send(m_internal_socket_,
                            m_pending_send_head_->buffer + m_pending_send_head_->bytes_sent,
                            m_pending_send_head_->buffer_size -  m_pending_send_head_->bytes_sent,
                            0);
#else
                        signal(SIGPIPE, SIG_IGN);
                        bytes_sent = ::send(m_internal_socket_,
                            m_pending_send_head_->buffer + m_pending_send_head_->bytes_sent,
                            m_pending_send_head_->buffer_size -  m_pending_send_head_->bytes_sent,
                            0);
#endif
                    } else {
                        dest.sin_addr.s_addr = m_pending_send_head_->remote_address;
                        dest.sin_port = htons(m_pending_send_head_->remote_port);
                        dest.sin_family = AF_INET;
#if defined(MSG_NOSIGNAL)
                        bytes_sent = ::sendto(m_internal_socket_,
                            m_pending_send_head_->buffer + m_pending_send_head_->bytes_sent,
                            m_pending_send_head_->buffer_size - m_pending_send_head_->bytes_sent,
                            MSG_NOSIGNAL, (struct sockaddr*)&dest,destlen);
#elif defined(WIN32)
                        bytes_sent = ::sendto(m_internal_socket_,
                            m_pending_send_head_->buffer + m_pending_send_head_->bytes_sent,
                            m_pending_send_head_->buffer_size - m_pending_send_head_->bytes_sent,
                            0, (struct sockaddr*)&dest,destlen);
#else
                        signal(SIGPIPE, SIG_IGN);
                        bytes_sent = ::sendto(m_internal_socket_,
                            m_pending_send_head_->buffer + m_pending_send_head_->bytes_sent,
                            m_pending_send_head_->buffer_size - m_pending_send_head_->bytes_sent,
                            0, (struct sockaddr*)&dest,destlen);
#endif

                    }
                    if (bytes_sent > 0) {
                        m_pending_bytes_to_send_ -= bytes_sent;
                        m_total_bytes_sent_ += bytes_sent;
                        m_pending_send_head_->bytes_sent += bytes_sent;
                        if (m_pending_send_head_->bytes_sent == m_pending_send_head_->buffer_size) {
                            if (m_pending_send_head_ == m_pending_send_tail_) {
                                m_pending_send_tail_ = NULL;
                            }
                            if (m_pending_send_head_->user_free ==0) {
                                delete (m_pending_send_head_->buffer);
                            }
                            temp = m_pending_send_head_->next;
                            delete m_pending_send_head_;
                            m_pending_send_head_ = temp;
                            if (m_pending_send_head_ == NULL) {
                                try_to_send = 0;
                            }
                        } else {
                            try_to_send = 1;
                        }
                    }
                    if (bytes_sent == -1) {
                        try_to_send = 0;
#if defined(_WIN32_WCE) || defined(WIN32)
                        bytes_sent = WSAGetLastError();
                        if (bytes_sent != WSAEWOULDBLOCK)
#elif defined(_POSIX)
                        if (errno != EWOULDBLOCK)
#endif
                        {
                            _clear_pending_send();
                            //ILibLifeTime_Add(module->LifeTime,socketModule,0,&ILibAsyncSocket_Disconnect,NULL);
                        }
                    }
                }

                if (m_pending_send_head_ == NULL && bytes_sent != -1) {
                    trigger_send_ok = 1;
                }
                m_send_lock_.unlock();
                if (trigger_send_ok != 0) {
                    m_on_send_OK_(this, m_user_);
                }
            } else {
                m_send_lock_.unlock();
            }

            m_send_lock_.lock();

            if (m_internal_socket_ != ~0) {
                if (m_fin_connect_ == 0) {
                    if (FD_ISSET(m_internal_socket_, (fd_set *)writeset) != 0) {
                        //m_timeout_timer_.remove();
                        getsockname(m_internal_socket_, (struct sockaddr*)&receiving_address_, &receiving_addr_len);
                        m_local_IP_address = receiving_address_.sin_addr.s_addr;
                        m_fin_connect_ = 1;
                        m_pause_ = 0;

#if defined(_WIN32_WCE) || defined(WIN32)
                        flags = 1;
                        ioctlsocket(m_internal_socket_, FIONBIO, (u_long *)&flags);
#elif defined(_POSIX)
                        flags = fcntl(m_internal_socket_, F_GETFL, 0);
                        fcntl(m_internal_socket_, F_SETFL, O_NONBLOCK|flags);
#endif
                        trigger_write_set = 1;
                    }
                    if (FD_ISSET(m_internal_socket_, (fd_set *)errorset) != 0) {
                        // Connection was a failure, so remove the timeout timer
                        //ILibLifeTime_Remove(module->TimeoutTimer,module);

#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)    
                        shutdown(m_internal_socket_, SD_BOTH);
#endif
                        closesocket(m_internal_socket_);
#elif defined(_POSIX)
                        shutdown(m_internal_socket_, SHUT_RDWR);
                        close(m_internal_socket_);
#endif
                        m_internal_socket_ = ~0;
                        trigger_error_set = 1;
                    }

                    m_send_lock_.unlock();

                    if (trigger_error_set != 0 && m_on_connect_ != NULL) {
                        m_on_connect_(this, 0, m_user_);
                    } else if (trigger_write_set != 0 && m_on_connect_ != NULL) {
                        m_on_connect_(this, -1, m_user_);
                    }
                } else {
                    if (FD_ISSET(m_internal_socket_, (fd_set *)errorset) != 0) {
                        if (m_current_QOS_priorty_ != QOS_NONE) {
                            unitialize_QOS();
                        }
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
                        shutdown(m_internal_socket_, SD_BOTH);
#endif
                        closesocket(m_internal_socket_);
#elif defined(_POSIX)
                        shutdown(m_internal_socket_, SHUT_RDWR);
                        close(m_internal_socket_);
#endif
                        m_internal_socket_ = ~0;
                        m_pause_ = 1;

                        _clear_pending_send();

                        trigger_error_set = 1;
                    }

                    /* Already Connected, just needs reading */
                    if (FD_ISSET(m_internal_socket_, (fd_set *)readset) != 0) {
                        trigger_read_set = 1;
                    } else if (m_pause_ < 0) {
                        // Someone resumed a paused connection, but the FD_SET was not triggered
                        // because there is no new data on the socket.
                        trigger_resume = 1;
                        ++m_pause_;
                    }

                    m_send_lock_.unlock();

                    if (trigger_error_set != 0 && m_on_disconnect_ != NULL) {
                        m_on_disconnect_(this, m_user_);
                    }
                    if (trigger_read_set != 0 || trigger_resume != 0) {
                        //ILibProcessAsyncSocket(this, trigger_read_set);
                    }
                }
            } else {
                m_send_lock_.unlock();
            }
        }

        void AsyncSocket::destroy(IObject * object)
        {
            SendData * temp, * current;

            if (!is_free()) {
                if (m_on_interrupt_ != NULL) {
                    m_on_interrupt_(this, m_user_);
                }
            }

            // Close socket if necessary
            if (m_internal_socket_ != ~0) {
                if (m_current_QOS_priorty_ != QOS_NONE) {
                    unitialize_QOS();
                }
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
                shutdown(m_internal_socket_, SD_BOTH);
#endif
                closesocket(m_internal_socket_);
#elif defined(_POSIX)
                shutdown(m_internal_socket_, SHUT_RDWR);
                close(m_internal_socket_);
#endif
            }

            // Free the buffer if necessary
            if (m_buffer_ != NULL) {
                delete m_buffer_;
                m_buffer_ = NULL;
                m_malloc_size_ = 0;
            }

            // Clear all the data that is pending to be sent
            temp = current = m_pending_send_head_;
            while (current != NULL) {
                temp = current->next;
                if (current->user_free == 0) {
                    delete current->buffer;
                }
                delete current;
                current = temp;
            }
        }

        void AsyncSocket::_clear_pending_send()
        {
            SendData * data, * temp;

            data = m_pending_send_head_;
            m_pending_send_tail_ = NULL;
            while (data != NULL) {
                temp = data->next;
                if (data->user_free == 0) {
                    delete (data->buffer);
                }
                delete (data);
                data = temp;
            }
            m_pending_send_head_ = NULL;
            m_pending_bytes_to_send_ = 0;
        }

        void AsyncSocket::_process_async_socket()
        {

        }
    }
}
