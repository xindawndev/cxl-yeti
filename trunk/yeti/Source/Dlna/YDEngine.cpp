#include "YDEngine.h"

#if (defined(_WIN32) || defined(_WIN32_WCE) || defined(_XBOX)) && !defined(__SYMBIAN32__)
#   if !defined(__WINSOCK__)
#       define __WINSOCK__
#   endif
#endif

#if defined(__WINSOCK__) &&!defined(_XBOX)
#   define STRICT
#   define YETI_WIN32_USE_WINSOCK2
#   ifdef YETI_WIN32_USE_WINSOCK2
#       include <winsock2.h>
#       include <ws2tcpip.h>
#   else
#       include <winsock.h>
#   endif
#   include <windows.h>
#   include "YetiConfig.h"
#   include "YetiWin32Network.h"
NAMEBEG
static WinsockSystem & WinsockInitializer = WinsockSystem::Initializer;
NAMEEND
#else
#   include <sys/types.h>
#   include <netinet/in.h>
#   include <netdb.h>
#   include <errno.h>
#endif

USINGNAMESPACE2;

template <> yeti::dlna::Engine * Singleton<yeti::dlna::Engine>::m_singleton_ = NULL;

namespace yeti
{
    namespace dlna
    {
        Engine & Engine::get_singleton(void) {
            YETI_ASSERT(m_singleton_);
            return (*m_singleton_);
        }

        Engine * Engine::get_singleton_ptr(void) {
            return m_singleton_;
        }

        Engine::Engine()
            : m_started_(false)
        {
            
        }

        void Engine::start()
        {
            stop();
            m_started_ = true;
            while (m_started_) {
                List<IObject *>::iterator it;
                for (it = m_objects_.get_first_item(); it; ++it) {
                    //(*it)->pre_process(*it, );
                }
                //select();
                for (it = m_objects_.get_first_item(); it; ++it) {
                    //(*it)->post_process(*it, );
                }
            }
        }

        void Engine::stop()
        {
            m_started_ = false;
            List<IObject *>::iterator it;
            for (it = m_objects_.get_first_item(); it; ++it) {
                (*it)->destroy(*it);
            }
            m_objects_.clear();
        }
    }
}
