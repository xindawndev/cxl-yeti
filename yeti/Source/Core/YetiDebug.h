#ifndef _CXL_YETI_DEBUG_H_
#define _CXL_YETI_DEBUG_H_

#include "YetiConfig.h"

#if defined(YETI_CONFIG_HAVE_ASSERT_H) && defined(YETI_DEBUG)
#include <assert.h>
#define YETI_ASSERT(x) assert(x)
#else
#define YETI_ASSERT(x) ((void)0)
#endif

extern void yeti_debug(const char* format, ...);
extern void yeti_debug_output(const char* message);

#endif // _CXL_YETI_DEBUG_H_
