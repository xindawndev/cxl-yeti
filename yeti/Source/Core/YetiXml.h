#ifndef _CXL_YETI_XML_H_
#define _CXL_YETI_XML_H_

#include "YetiTypes.h"
#include "YetiList.h"
#include "YetiString.h"
#include "YetiStreams.h"

NAMEBEG

const int YETI_ERROR_XML_INVALID_NESTING = YETI_ERROR_BASE_XML - 0;
const int YETI_ERROR_XML_TAG_MISMATCH    = YETI_ERROR_BASE_XML - 1;
const int YETI_ERROR_XML_NO_ROOT         = YETI_ERROR_BASE_XML - 2;
const int YETI_ERROR_XML_MULTIPLE_ROOTS  = YETI_ERROR_BASE_XML - 3;

#define YETI_XML_ANY_NAMESPACE "*"
#define YETI_XML_NO_NAMESPACE  NULL

class XmlProcessor;

class XmlAttribute
{
public:
private:
};

class XmlNamespaceMap
{
public:
private:
};

class XmlElementNode;
class XmlTextNode;

class XmlNode
{
public:
private:
};

class XmlElementNode : public XmlNode
{
public:
private:
};

class XmlTextNode : public XmlNode
{
public:
private:
};

class XmpParser
{
public:
private:
};

class XmlSerializer
{
public:
private:
};

class XmlWriter
{
public:
    explicit XmlWriter(YETI_Cardinal indentation = 0) : m_indentation_(indentation) {}

    YETI_Result serialize(XmlNode & node, OutputStream & stream, bool add_xml_decl = false);

private:
    YETI_Cardinal m_indentation_;
};

class XmlCanonicalizer
{
public:
    YETI_Result serialize(XmlNode & node, OutputStream & stream, bool add_xml_decl = false);
};

NAMEEND

#endif // _CXL_YETI_XML_H_
