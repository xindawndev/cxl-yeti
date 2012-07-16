#include "YetiXml.h"

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiUtil.h"
#include "YetiMap.h"
#include "YetiDebug.h"

//#define YETI_XML_PARSER_DEBUG
#ifdef YETI_XML_PARSER_DEBUG
#define YETI_XML_Debug_0(s) YETI_Debug(s)
#define YETI_XML_Debug_1(s,x0) YETI_Debug(s,x0)
#define YETI_XML_Debug_2(s,x0,x1) YETI_Debug(s,x0,x1)
#define YETI_XML_Debug_3(s,x0,x1,x2) YETI_Debug(s,x0,x1,x2)
#define YETI_XML_Debug_4(s,x0,x1,x2,x3) YETI_Debug(s,x0,x1,x2,x3)
#else
#define YETI_XML_Debug_0(s)
#define YETI_XML_Debug_1(s,x0)
#define YETI_XML_Debug_2(s,x0,x1)
#define YETI_XML_Debug_3(s,x0,x1,x2)
#define YETI_XML_Debug_4(s,x0,x1,x2,x3)
#endif

NAMEBEG

static const String YETI_XmlNamespaceUri_Xml("http://www.w3.org/XML/1998/namespace");

NAMEEND
