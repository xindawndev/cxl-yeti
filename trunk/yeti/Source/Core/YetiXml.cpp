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

class XmlAttributeFinder
{
public:
    XmlAttributeFinder(const XmlElementNode & element,
        const char * name,
        const char * namespc)
        : m_element_(element), m_name_(name), m_namespace_(namespc) {}

    bool operator()(const XmlAttribute * const & attribute) const
    {
        if (attribute->m_name_ == m_name_) {
            if (m_namespace_) {
                const String & prefix = attribute->get_prefix();
                if (m_namespace_[0] == '\0') {
                    return prefix.is_empty();
                } else {
                    if (prefix.is_empty()) {
                        return false;
                    } else {
                        const String * namespc = m_element_.get_namespace_uri(prefix);
                        return namespc && *namespc == m_namespace_;
                    }
                }
            } else {
                return true;
            }
        } else {
            return false;
        }
    }

private:
    const XmlElementNode & m_element_;
    const char * m_name_;
    const char * m_namespace_;
};

class XmlAttributeFinderWithPrefix
{
public:
    XmlAttributeFinderWithPrefix(const char * prefix, const char * name)
        : m_prefix_(prefix ? prefix : ""), m_name_(name) {}

    bool operator()(const XmlAttribute * const & attribute) const {
        return attribute->m_prefix_ == m_prefix_ && attribute->m_name_ == m_name_;
    }
private:
    const char * m_prefix_;
    const char * m_name_;
};

class XmlTagFinder
{
public:
    XmlTagFinder(const char * tag, const char * namespc)
        : m_tag_(tag), m_namespace_(namespc) {}

    bool operator()(const XmlNode * const & node) const {
        const XmlElementNode * element = node->as_element_node();
        if (element && element->m_tag_ == m_tag_) {
            if (m_namespace_) {
                const String * namespc = element->get_namespace();
                if (namespc) {
                    return * namespc == m_namespace_;
                } else {
                    return m_namespace_[0] == '\0';
                }
            } else {
                return true;
            }
        } else {
            return false;
        }
    }

private:
    const char * m_tag_;
    const char * m_namespace_;
};

class XmlTextFinder
{
public:
    bool operator()(const XmlNode * const & node) const {
        return node->as_text_node() != NULL;
    }
};

class XmlNamespaceCollapser
{
public:
    XmlNamespaceCollapser(XmlElementNode * element)
        : m_root_(element) {}

    void operator()(XmlNode *& node) const {
        XmlElementNode * element = node->as_element_node();
        if (element == NULL) return;
        _collapse_namespace(element, element->get_prefix());
        List<XmlAttribute *>::iterator item = element->get_attributes().get_first_item();
        while (item) {
            XmlAttribute * attribute = *item;
            _collapse_namespace(element, attribute->get_prefix());
            ++item;
        }
        element->get_children().apply(*this);
    }
private:
    void _collapse_namespace(XmlElementNode * element, const String & prefix) const;
    XmlElementNode * m_root_;
};

void XmlNamespaceCollapser::_collapse_namespace(XmlElementNode * element, const String & prefix) const
{
    if (m_root_->m_namespace_map_ == NULL ||
        (m_root_->m_namespace_map_->get_namespace_uri(prefix) == NULL && prefix != "xml")) {
            const String * uri = element->get_namespace_uri(prefix);
            if (uri) m_root_->set_namespace_uri(prefix, uri->get_chars());
    }
}

XmlAttribute::XmlAttribute(const char * name, const char * value)
: m_value_(value)
{
    const char * cursor = name;
    while (char c = *cursor++) {
        if (c == ':') {
            unsigned int prefix_length = (unsigned int)(cursor - name) - 1;
            m_prefix_.assign(name, prefix_length);
            name = cursor;
            break;
        }
    }
    m_name_ = name;
}

XmlElementNode::XmlElementNode(const char * prefix, const char * tag)
: XmlNode(ELEMENT)
, m_prefix_(prefix)
, m_tag_(tag)
, m_namespace_map_(NULL)
, m_namespace_parent_(NULL)
{
}

XmlElementNode::XmlElementNode(const char * tag)
: XmlNode(ELEMENT)
, m_namespace_map_(NULL)
, m_namespace_parent_(NULL)
{
    const char * cursor = tag;
    while (char c = *cursor++) {
        if (c == ':') {
            unsigned int prefix_length = (unsigned int)(cursor - tag) - 1;
            m_prefix_.assign(tag, prefix_length);
            tag = cursor;
            break;
        }
    }
    m_tag_ = tag;
}

XmlElementNode::~XmlElementNode()
{
    m_children_.apply(ObjectDeleter<XmlNode>());
    m_attributes_.apply(ObjectDeleter<XmlAttribute>());
    delete m_namespace_map_;
}

void XmlElementNode::set_parent(XmlNode * parent)
{
    m_parent_ = parent;
    XmlElementNode * parent_element = parent ? parent->as_element_node() : NULL;
    XmlElementNode * namespace_parent;
    if (parent_element) {
        namespace_parent = parent_element->m_namespace_map_ ? parent_element : parent_element->m_namespace_parent_;
    } else {
        namespace_parent = NULL;
    }
    if (namespace_parent != m_namespace_parent_) {
        m_namespace_parent_ = namespace_parent;
        relink_namespace_maps();
    }
}

YETI_Result XmlElementNode::add_child(XmlNode * child)
{
    if (child == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    child->set_parent(this);
    return m_children_.add(child);
}

XmlElementNode * XmlElementNode::get_child(const char * tag, const char * namespc, YETI_Ordinal n) const
{
    if (namespc == NULL || namespc[0] == '\0') {
        namespc = "";
    } else if (namespc[0] == '*' && namespc[1] == '\0') {
        namespc = NULL;
    }

    List<XmlNode *>::iterator item;
    item = m_children_.find(XmlTagFinder(tag, namespc), n);
    return item ? (*item)->as_element_node() : NULL;
}

YETI_Result XmlElementNode::add_attribute(const char * name, const char * value)
{
    if (name == NULL || value == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    return m_attributes_.add(new XmlAttribute(name, value));
}

YETI_Result XmlElementNode::set_attribute(const char * prefix, const char * name, const char * value)
{
    if (name == NULL || value == NULL) return YETI_ERROR_INVALID_PARAMETERS;
    List<XmlAttribute *>::iterator attribute;
    attribute = m_attributes_.find(XmlAttributeFinderWithPrefix(prefix, name));
    if (attribute) {
        (*attribute)->set_value(value);
        return YETI_SUCCESS;
    }
    return m_attributes_.add(new XmlAttribute(prefix, name, value));
}


YETI_Result XmlElementNode::set_attribute(const char * name, const char * value)
{
    return set_attribute(NULL, name, value);
}
NAMEEND
