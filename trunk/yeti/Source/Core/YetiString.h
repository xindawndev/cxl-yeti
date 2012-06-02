#ifndef _CXL_YETI_STRING_H_
#define _CXL_YETI_STRING_H_

#include "YetiConfig.h"
#if defined(YETI_CONFIG_HAVE_NEW_H)
#   include <new>
#endif
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiList.h"
#include "YetiArray.h"
#include "YetiDebug.h"
#include "YetiHash.h"

NAMEBEG

const int YETI_STRING_SEARCH_FAILED = -1;

class String
{
public:
};

NAMEEND

#endif // _CXL_YETI_STRING_H_
