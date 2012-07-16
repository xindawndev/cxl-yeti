#include "YetiWin32Thread.h"

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiThreads.h"
#include "YetiDebug.h"
#include "YetiResults.h"
#include "YetiTime.h"
#include "YetiSystem.h"

NAMEBEG

Mutex::Mutex()
{
    m_delegate_ = new Win32Mutex();
}

Win32Mutex::Win32Mutex()
{
    m_handle_ = CreateMutex(NULL, FALSE, NULL);
}

Win32Mutex::~Win32Mutex()
{
    CloseHandle(m_handle_);
}

YETI_Result Win32Mutex::lock()
{
    DWORD result = WaitForSingleObject(m_handle_, INFINITE);
    if (result == WAIT_OBJECT_0) {
        return YETI_SUCCESS;
    }

    return YETI_FAILURE;
}

YETI_Result Win32Mutex::unlock()
{
    ReleaseMutex(m_handle_);
    return YETI_SUCCESS;
}

NAMEEND
