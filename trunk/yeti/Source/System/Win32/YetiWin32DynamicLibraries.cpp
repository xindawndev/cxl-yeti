#include "YetiLogging.h"
#include "YetiDynamicLibraries.h"

#include <windows.h>
#include <assert.h>

YETI_SET_LOCAL_LOGGER("yeti.win32.dynamic-libraries")

NAMEBEG

static LPWSTR a2w_helper(LPWSTR lpw, LPCSTR lpa, int nchars, UINT acp)
{
    int ret;

    assert(lpa != NULL);
    assert(lpw != NULL);
    if (lpw == NULL || lpa == NULL) return NULL;

    lpw[0] = '\0';
    ret = ::MultiByteToWideChar(acp, 0, lpa, -1, lpw, nchars);
    if (ret == 0) {
        assert(0);
        return NULL;
    }
    return lpw;
}

#if !defined(_XBOX)
#define YETI_WIN32_USE_CHAR_CONVERSION int _convert = 0; LPCWSTR _lpw = NULL; LPCSTR _lpa = NULL;
#define  YETI_WIN32_A2W(lpa) (\
    ((_lpa = lpa) == NULL) ? NULL : (\
    _convert = (int)(strlen(_lpa) + 1), \
    (INT_MAX / 2 < _convert) ? NULL : \
    a2w_helper((LPWSTR)alloca(_convert * sizeof(WCHAR)), _lpa, _convert, CP_UTF8)))
#else
#define YETI_WIN32_USE_CHAR_CONVERSION
#define YETI_WIN32_A2W(_s) (_s)
#define LoadLibraryW LoadLibrary
#define GetProcAddressW GetProcAddress
#endif

class Win32DynamicLibrary : public DynamicLibraryInterface
{
public:
    Win32DynamicLibrary(HMODULE library, const char * name)
        : m_library_(library), m_name_(name) {}
    virtual YETI_Result find_symbol(const char * name, void *& symbol);
    virtual YETI_Result unload();

private:
    HMODULE m_library_;
    String m_name_;
};

YETI_Result DynamicLibrary::load(const char * name, YETI_Flags flags, DynamicLibrary *& library)
{
    YETI_WIN32_USE_CHAR_CONVERSION;
    YETI_COMPILER_UNUSED(flags);
    if (name == NULL) return YETI_ERROR_INVALID_PARAMETERS;

    library = NULL;
    YETI_LOG_FINE_2("loading library %s, flags = %x", name, flags);
    HMODULE handle = LoadLibraryW(YETI_WIN32_A2W(name));
    if (handle == NULL) {
        YETI_LOG_FINE("library not found");
        return YETI_FAILURE;
    }

    YETI_LOG_FINE_1("library %s loaded", name);
    library = new DynamicLibrary(new Win32DynamicLibrary(handle, name));
    return YETI_SUCCESS;
}

YETI_Result Win32DynamicLibrary::find_symbol(const char * name, void *& symbol)
{
    if (name == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    symbol = NULL;
    if (m_library_ == NULL) return YETI_ERROR_NO_SUCH_ITEM;

    YETI_LOG_FINE_1("finding symbol %s", name);
#if defined(_WIN32_WCE)
    YETI_WIN32_USE_CHAR_CONVERSION;
    symbol = GetProcAddress(m_library_, YETI_WIN32_A2W(name));
#else
    symbol = GetProcAddress(m_library_, name);
#endif
    return symbol ? YETI_SUCCESS : YETI_ERROR_NO_SUCH_ITEM;
}

YETI_Result Win32DynamicLibrary::unload()
{
    YETI_LOG_FINE_1("unloading library %s", (const char *)m_name_);
    BOOL result = FreeLibrary(m_library_);
    if (result) {
        return YETI_SUCCESS;
    }
    return YETI_FAILURE;
}

NAMEEND
