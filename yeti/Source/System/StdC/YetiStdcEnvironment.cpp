#include <stdlib.h>

#include "YetiConfig.h"
#include "YetiUtil.h"
#include "YetiResults.h"

NAMEBEG

YETI_Result Environment::get(const char * name, String& value)
{
    char * env;

    value.set_length(0);

#if defined(YETI_CONFIG_HAVE_GETENV)
    env = getenv(name);
    if (env) {
        value = env;
        return YETI_SUCCESS;
    } else {
        return YETI_ERROR_NO_SUCH_ITEM;
    }
#elif defined(YETI_CONFIG_HAVE_DUPENV_S)
    if (dupenv_s(&env, NULL, name) != 0) {
        return YETI_FAILURE;
    } else if (env != NULL) {
        value = env;
        free(env);
        return YETI_SUCCESS;
    } else {
        return YETI_ERROR_NO_SUCH_ITEM;
    }
#else
    return YETI_ERROR_NOT_SUPPORTED;
#endif
}

YETI_Result Environment::set(const char * name, const char * value)
{
    int result = 0;
    if (value) {
#if defined(YETI_CONFIG_HAVE_SETENV)
        setenv(name, value, 1); 
#elif defined(YETI_CONFIG_HAVE_PUTENV_S)
        result = putenv_s(name, value);
#else
        return YETI_ERROR_NOT_SUPPORTED;
#endif
    } else {
#if defined(YETI_CONFIG_HAVE_UNSETENV)
        unsetenv(name); // ignore return value (some platforms have this function as void)
#elif defined(YETI_CONFIG_HAVE_PUTENV_S)
        result = putenv_s(name, "");
#else
        return YETI_ERROR_NOT_SUPPORTED;
#endif
    }
    return result == 0 ? YETI_SUCCESS : YETI_FAILURE;
}

NAMEEND
