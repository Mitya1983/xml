#ifndef XML_HPP
#define XML_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace xml {

class XmlElement
{
    friend std::ostream &operator<<(std::ostream &output, const XmlElement &xmlElement);
    friend std::stringstream &operator<<(std::stringstream &output, const XmlElement &xmlElement);
public:
    //CONSTRUCTORS
    XmlElement(std::string_view name);
    XmlElement(std::string_view name, std::string_view value);
    XmlElement(const XmlElement&) = delete;
    XmlElement(XmlElement&&) = default;
    //OPERATORS
    XmlElement& operator=(const XmlElement&) = delete;
    XmlElement& operator=(XmlElement&&) = default;
    //DESTRUCTOR
    ~XmlElement() = default;

    //API
    void addElement(XmlElement &&element);
    void addAttribute(std::string_view attributeName, std::string_view attributeValue);
    std::shared_ptr<XmlElement> find(const std::string &name);
    std::vector<std::shared_ptr<XmlElement>> equal_range(const std::string &name);
    std::string toString() const;
    std::string toString_b(uint8_t level = 0) const;

    static XmlElement fromXmlString(std::string_view xmlString);

private:
    std::string m_name;
    std::string m_value;
    std::vector<std::pair<std::string, std::string>> m_attributes;
    std::vector<std::shared_ptr<XmlElement>> m_childs;

    std::string _createOpenTag(bool beautify) const;
    std::string _createCloseTag(bool beautify) const;
    std::vector<std::pair<std::string, size_t> > _getOpenTags(std::string_view xmlData);
    static bool _hasChilds(std::string_view xmlData, std::string_view tagName);
    //SETTERS AND GETTERS
public:
    void setValue(std::string_view value);
    void setName(std::string_view name) {m_name = name;}
    const std::string &name() const noexcept {return m_name;}
    const std::vector<std::pair<std::string, std::string>> &attributes() const noexcept {return m_attributes;}
    const std::string &value() const noexcept {return m_value;}
    const std::vector<std::shared_ptr<XmlElement>> &childs() const noexcept {return m_childs;}
};

class Xml
{
    friend std::ostream &operator<<(std::ostream &output, const Xml &xmlElement);
    friend std::stringstream &operator<<(std::stringstream &output, const Xml &xmlElement);
public:
    //CONSTRUCTORS
    Xml(XmlElement &&root);
    explicit Xml(const std::string &xmlData);
    explicit Xml(const std::filesystem::path &xmlFile);
    Xml(const Xml&) = delete;
    Xml(Xml&&) = default;
    //OPERATORS
    Xml& operator=(const Xml&) = delete;
    Xml& operator=(Xml&&) = default;
    //DESTRUCTOR
    ~Xml() = default;

    //API
    std::string toString() const;
    auto find (const std::string &name) {return m_root.find(name);};
    auto equal_range (const std::string &name) {return m_root.equal_range(name);};

    static std::string getValueByTag(std::string_view xmlString, std::string_view tagName, size_t initialPos = 0);
    static std::string getXmlDataByTag(std::string_view xmlString, std::string_view tagName, size_t initialPos = 0);

protected:

private:
    XmlElement m_root;
    bool m_beautifyOutput;
    //SETTERS AND GETTERS
public:
    void setBeautifyOutput() {m_beautifyOutput = true;}

    const XmlElement &root() noexcept {return m_root;}

};

std::ostream &operator<<(std::ostream &output, const XmlElement &xmlElement);
std::stringstream &operator<<(std::stringstream &output, const XmlElement &xmlElement);
std::ostream &operator<<(std::ostream &output, const Xml &xmlElement);
std::stringstream &operator<<(std::stringstream &output, const Xml &xmlElement);

} // namespace zestad::xml
#endif // XML_HPP
