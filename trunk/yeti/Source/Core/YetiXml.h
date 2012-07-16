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
    XmlAttribute(const char * name, const char * value);
    XmlAttribute(const char * prefix, const char * name, const char * value)
        : m_prefix_(prefix)
        , m_name_(name)
        , m_value_(value) {}

    const String & get_prefix() const { return m_prefix_; }
    const String & get_name() const { return m_name_; }
    const String & get_value() const { return m_value_; }

    void set_value(const char * value) { m_value_ = value; }
private:
    String m_prefix_;
    String m_name_;
    String m_value_;

    XmlAttribute(const XmlAttribute & attribute)
        : m_prefix_(attribute.m_prefix_)
        , m_name_(attribute.m_name_)
        , m_value_(attribute.m_value_) {}
    XmlAttribute & operator=(const XmlAttribute & a);

    friend class XmlAttributeFinder;
    friend class XmlAttributeFinderWithPrefix;
};

class XmlNamespaceMap
{
public:
    ~XmlNamespaceMap();

    YETI_Result set_namespace_uri(const char * prefix, const char * uri);
    const String get_namespace_uri(const char * prefix);
    const String get_namespace_prefix(const char * uri);

private:
    class Entry {
    public:
        Entry(const char * prefix, const char * uri)
            : m_prefix_(prefix)
            , m_uri_(uri) {}

        String m_prefix_;
        String m_uri_;
    };
    List<Entry *> m_entries_;

    friend class XmlWriter;
    friend class XmlNodeWriter;
    friend class XmlNodeCanonicalWriter;
};

class XmlElementNode;
class XmlTextNode;

class XmlNode
{
public:
    typedef enum {
        DOCUMENT,
        ELEMENT,
        TEXT
    } Type;

    XmlNode(Type type) : m_type_(type), m_parent_(NULL) {}
    virtual ~XmlNode() {}
    Type get_type() const { return m_type_; }
    XmlNode * get_parent() const { return m_parent_; }

    virtual XmlElementNode * as_element_node() { return NULL; }
    virtual const XmlElementNode * as_element_node() const { return NULL; }
    virtual XmlTextNode * as_text_node() { return NULL; }
    virtual const XmlTextNode * as_text_node() const { return NULL; }

protected:
    virtual void set_parent(XmlNode * parent) { m_parent_ = parent; }

    Type m_type_;
    XmlNode * m_parent_;

    friend class XmlNodeFinder;
    friend class XmlSerializer;
    friend class XmlWriter;
    friend class XmlElementNode;
};

class XmlElementNode : public XmlNode
{
public:
    XmlElementNode(const char * tag);
    XmlElementNode(const char * prefix, const char * tag);

    virtual ~XmlElementNode();

    List<XmlNode *> & get_children() { return m_children_; }
    const List<XmlNode *> & get_children() const { return m_children_; }
    XmlElementNode * get_child(const char * tag,
        const char * namespc = YETI_XML_NO_NAMESPACE,
        YETI_Ordinal n = 0) const;
    YETI_Result add_child(XmlNode * child);
    YETI_Result set_attribute(const char * prefix,
        const char * name,
        const char * value);
    YETI_Result set_attribute(const char * name,
        const char * value);
    YETI_Result add_text(const char * text);
    List<XmlAttribute *> & get_attributes() { return m_attributes_; }
    const List<XmlAttribute *> & get_attributes() const { return m_attributes_; }
    const String & get_attribute(const char * name,
        const char * namespc = YETI_XML_NO_NAMESPACE) const;
    const String & get_prefix() const { return m_prefix_; }
    const String & get_tag() const { return m_tag_; }
    const String & get_text(YETI_Ordinal n = 0) const;

    YETI_Result make_standalone();

    const String * get_namespace() const;
    YETI_Result set_namespace_uri(const char * prefix, const char * uri);
    const String * get_namespace_uri(const char * prefix) const;
    const String * get_namespace_prefix(const char * uri) const;

    XmlElementNode * as_element_node() { return this; }
    const XmlElementNode * as_element_node() const { return this; }

protected:
    void set_parent(XmlNode * parent);
    void set_namespace_parent(XmlElementNode * parent);
    void relink_namespace_maps();

    YETI_Result add_attribute(const char * name, const char * value);

    String                  m_prefix_;
    String                  m_tag_;
    List<XmlNode *>         m_children_;
    List<XmlAttribute *>    m_attributes_;
    XmlNamespaceMap *       m_namespace_map_;
    XmlElementNode *        m_namespace_parent_;

    friend class XmlTagFinder;
    friend class XmlSerializer;
    friend class XmlWriter;
    friend class XmlNodeWriter;
    friend class XmlNodeCanonicalWriter;
    friend class XmlParser;
    friend class XmlProcessor;
    friend class XmlNamespaceCollapser;
};

class XmlTextNode : public XmlNode
{
public:
    typedef enum {
        CHARACTER_DATA,
        IGNORABLE_WHITESPACE,
        CDATA_SECTION,
        ENTITY_REFERENCE,
        COMMENT
    } TokenType;

    XmlTextNode(TokenType token_type, const char * text);
    const String & get_string() const { return m_text_; }
    XmlTextNode * as_text_node() { return this; }
    const XmlTextNode * as_text_node() const { return this; }

private:
    TokenType m_token_type_;
    String m_text_;

};

class XmlParser
{
public:
    XmlParser(bool keep_whitespace = true);
    virtual ~XmlParser();

    virtual YETI_Result parse(const char * xml,
        XmlNode *& tree,
        bool incremental = false);

    virtual YETI_Result parse(const char * xml,
        YETI_Size size,
        XmlNode *& tree,
        bool incremental = false);

    virtual YETI_Result parse(InputStream & stream,
        XmlNode *& tree,
        bool incremental = false);

    virtual YETI_Result parse(InputStream & stream,
        YETI_Size size,
        XmlNode *& tree,
        bool incremental = false);

protected:
    YETI_Result on_start_element(const char * name);
    YETI_Result on_element_attribute(const char * name, const char * value);
    YETI_Result on_end_element(const char * name);
    YETI_Result on_character_data(const char * data, unsigned long size);
    void remove_ignorable_whitespace();

    XmlProcessor * m_processor_;
    XmlElementNode * m_root_;
    XmlElementNode * m_current_element_;
    bool m_keep_whitespace_;

private:
    void _reset();
    friend class XmlProcessor;
};

class XmlSerializer
{
public:
    XmlSerializer(OutputStream * output,
        YETI_Cardinal indentation = 0,
        bool shrink_empty_element = true,
        bool add_xml_decl = false);

    virtual ~XmlSerializer();
    virtual YETI_Result start_document();
    virtual YETI_Result end_document();
    virtual YETI_Result start_element(const char * prefix, const char * name);
    virtual YETI_Result end_element(const char * prefix, const char * name);
    virtual YETI_Result attribute(const char * prefix, const char * name, const char * value);
    virtual YETI_Result text(const char * text);
    virtual YETI_Result cdata_section(const char * data);
    virtual YETI_Result comment(const char * comment);

protected:
    void escape_char(unsigned char c, char * text);
    YETI_Result process_pending();
    YETI_Result output_escaped_string(const char * text, bool attribute);
    void output_indentation(bool start);

    OutputStream * m_output_;
    bool m_element_pendinig_;
    YETI_Cardinal m_depth_;
    YETI_Cardinal m_indentation_;
    String m_indentation_prefix_;
    bool m_element_has_text_;
    bool m_shrink_empty_elements_;
    bool m_add_xml_decl_;
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
