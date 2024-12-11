#include "xml.hpp"

#include <fstream>
#include <filesystem>
#include <cassert>

#if defined (_WIN32) || defined(_WIN64)
#include <cerrno.h>
#endif

namespace {

const std::string xmlHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n";

} // namespace

mt::xml::XmlElement::XmlElement(std::string p_name) :
    m_name(std::move(p_name))
{

}

mt::xml::XmlElement::XmlElement(std::string p_name, std::string p_value) :
    m_name(std::move(p_name)),
    m_value(std::move(p_value))
{

}

void mt::xml::XmlElement::addElement(mt::xml::XmlElement &&element)
{
    assert(m_value.empty() && "XmlElement may comrise either value either child XmlElement");
    m_childs.emplace_back(std::make_shared<mt::xml::XmlElement>(std::move(element)));

}

void mt::xml::XmlElement::addAttribute(std::string_view attributeName, std::string_view attributeValue)
{
    assert(!attributeName.empty() && "XmlElement: attribute name can't be empty");
    assert(!attributeValue.empty() && "XmlElement: attribute value can't be empty");
    m_attributes.emplace_back(attributeName, attributeValue);
}

auto mt::xml::XmlElement::find(const std::string& name) const -> std::shared_ptr< mt::xml::XmlElement > {
    for (const auto &element : m_childs){
        if (element->name() == name){
            return element;
        }
    }
    return nullptr;
}

auto mt::xml::XmlElement::equal_range(const std::string& name) -> std::vector< std::shared_ptr< mt::xml::XmlElement > > {
    std::vector<std::shared_ptr<XmlElement>> result;
    for (const auto &element : m_childs){
        if (element->name() == name){
            result.push_back(element);
        }
    }
    return result;
}

auto mt::xml::XmlElement::toString() const -> std::string {
    std::string returnValue;

    returnValue += _createOpenTag(false);
    if (m_childs.empty()){
        if (m_value.empty()){
            return returnValue;
        }
        returnValue += m_value;
    }
    else{
        for (const auto &child : m_childs){
            returnValue += child->toString();
        }
    }
    returnValue += _createCloseTag(false);

    return returnValue;
}

auto mt::xml::XmlElement::toString_b(uint8_t level) const -> std::string {
    std::string returnValue;

    for (int levelCount = 0; levelCount < level; ++levelCount){
        returnValue += "  ";
    }
    returnValue += _createOpenTag(true);
    if (m_childs.empty()){
        if (m_value.empty()){
            return returnValue;
        }
        returnValue += m_value;
    }
    else{
        for (const auto &child : m_childs){
            returnValue += child->toString_b(level + 1);
        }
        for (int levelCount = 0; levelCount < level; ++levelCount){
            returnValue += "  ";
        }
    }
    returnValue += _createCloseTag(true);

    return returnValue;
}

auto mt::xml::XmlElement::fromXmlString(std::string_view xmlString) -> mt::xml::XmlElement {
    auto startPos = xmlString.find('<', 0) + 1;
    auto endPos = xmlString.find('>', startPos);
    std::string openTag(xmlString.substr(startPos, endPos - startPos));
    std::string tagName;
    bool selfClosingTag = false;
    std::vector<std::pair<std::string, std::string>> l_attributes;
    if (openTag.find('=') == std::string::npos){
        if (openTag.back() != '/'){
            tagName = openTag;
        }
        else{
            if (openTag.find(' ') != std::string::npos){
                tagName = openTag.substr(0, openTag.find(' '));
            }
            else{
                tagName = openTag.substr(0, openTag.find('/'));
            }
            selfClosingTag = true;
        }
    }
    else{
        size_t index = openTag.find(' ');
        tagName = openTag.substr(0, index);
        ++index;
        while(true){
            std::string attributeName;
            std::string attributeValue;
            if (index >= openTag.length()){
                break;
            }
            if (openTag.at(index) == ' '){
                ++index;
                continue;
            }
            else if (openTag.at(index) == '/'){
                selfClosingTag = true;
                break;
            }
            else {
                char c;
                while (((c = openTag.at(index)) != ' ') && ((c = openTag.at(index)) != '=')){
                    attributeName += c;
                    ++index;
                }
                auto searchStartPos = index;
                index = openTag.find('\'', searchStartPos);
                if (index == std::string::npos){
                    index = openTag.find('\"', searchStartPos);
                }
                if (index == std::string::npos){
                    throw std::runtime_error("zestad::mt::xml::XmlElement zestad::mt::xml::XmlElement::fromXmlString: xmlData is corrupted due to attribute in tag " + tagName);
                }
                ++index;
                while (((c = openTag.at(index)) != '\'') && ((c = openTag.at(index)) != '\"')){
                    attributeValue += c;
                    ++index;
                }
                index += 1;
                l_attributes.emplace_back(attributeName, attributeValue);
            }
        }
//        if (openTag.back() == '/'){
//            selfClosingTag = true;
//        }
    }

    mt::xml::XmlElement element(tagName);
    for (const auto &attribute : l_attributes){
        element.addAttribute(attribute.first, attribute.second);
    }
    if (!selfClosingTag){
        if (!_hasChilds(xmlString, tagName)){
            element.setValue(mt::xml::getValueByTag(xmlString, tagName));
        }
        else{
            std::string xmlValue = mt::xml::getValueByTag(xmlString, tagName);
            for (const auto &l_tagName : element._getOpenTags(xmlValue)){
                std::string childXmlString = mt::xml::getXmlDataByTag(xmlValue, l_tagName.first, l_tagName.second);
                element.addElement(mt::xml::XmlElement::fromXmlString(childXmlString));
            }
        }
    }
    return element;
}

std::string mt::xml::XmlElement::_createOpenTag(bool beautify) const
{
    std::string returnValue = "<";
    returnValue += m_name;
    if (!m_attributes.empty()){
        for (const auto &attribute : m_attributes){
            returnValue += ' ';
            returnValue += attribute.first;
            returnValue += "=\'";
            returnValue += attribute.second;
            returnValue += '\'';
        }
    }
    if (m_childs.empty() && m_value.empty()){
        returnValue += "/>";
        if (beautify){
            returnValue += "\n";
        }
    }
    else{
        returnValue += '>';
    }

    if (!m_childs.empty() && beautify){
        returnValue += '\n';
    }
    return returnValue;
}

std::string mt::xml::XmlElement::_createCloseTag(bool beautify) const
{
    std::string returnValue = "</";
    returnValue += m_name;
    returnValue += ">";
    if (beautify){
        returnValue += "\n";
    }
    return returnValue;
}

std::vector<std::pair<std::string, size_t>> mt::xml::XmlElement::_getOpenTags(std::string_view xmlData)
{
    size_t startPos = 0;
    std::vector<std::pair<std::string, size_t>> tagNames;

    while (true){
        startPos = xmlData.find('<', startPos);
        if (startPos == std::string::npos){
            break;
        }
        ++startPos;
        auto endPos = xmlData.find('>', startPos);
        std::string openTag(xmlData.substr(startPos, endPos - startPos));
        std::string tagName;
        if (openTag.find('=') == std::string::npos && openTag.back() != '/'){
            tagName = openTag;
        }
        else{
            if (openTag.find(' ') != std::string::npos){
                tagName = openTag.substr(0, openTag.find(' '));
            }
            else{
                tagName = openTag.substr(0, openTag.find('/'));
            }
        }
        tagNames.push_back(std::make_pair(tagName, startPos - 1));
        if (openTag.back() != '/')
        {
            std::string closeTag = "</" + tagName + '>';
            startPos = xmlData.find(closeTag, startPos) + closeTag.length();
        }
        else{
            startPos = endPos + 1;
        }
    }

    return tagNames;
}

bool mt::xml::XmlElement::_hasChilds(std::string_view xmlData, std::string_view tagName)
{
    if (mt::xml::getValueByTag(xmlData, tagName).find('<') != std::string::npos){
        return true;
    }

    return false;
}


void mt::xml::XmlElement::setValue(std::string_view value)
{
    assert(m_childs.empty() && "XmlElement may comrise either value either child XmlElement");
    m_value = value;
}

mt::xml::Xml(mt::xml::XmlElement &&root) :
    m_root(std::move(root))
{

}

mt::xml::Xml(const std::string &xmlData) :
    m_root("")
{
    auto headerEndPos = xmlData.find("?>") + 2;
    std::string xmlString = xmlData.substr(headerEndPos);

    m_root = mt::xml::XmlElement::fromXmlString(xmlString);
}

mt::xml::Xml(const std::filesystem::path &xmlFile) :
    m_root("")
{
    std::ifstream file;
    file.open(xmlFile);
    if (!file.is_open()){
        std::string msg = "Could not open xml file: " + std::error_code(errno, std::system_category()).message();
        throw std::runtime_error(msg);
    }

    std::stringstream stream;
    stream << file.rdbuf();
    std::string xmlData = stream.str();

    *this = mt::xml::Xml(xmlData);

    file.close();
}

std::string mt::xml::toString() const
{
    std::string returnValue = xmlHeader;
    if (m_beautifyOutput){
        returnValue += m_root.toString_b();
    }
    else{
        returnValue += m_root.toString();
    }

    return returnValue;
}

std::string mt::xml::Xml::getValueByTag(std::string_view xmlString, std::string_view tagName, size_t initialPos)
{
    std::string openTag("<");
    openTag += tagName;
    auto openTagStartPos = xmlString.find(tagName, initialPos);
    if (openTagStartPos == std::string::npos){
        return std::string();
    }
    openTagStartPos += tagName.length();
    auto openTagEndPos = xmlString.find('>', openTagStartPos) + 1;

    std::string closeTag("</");
    closeTag += tagName;
    closeTag += '>';

    auto valueEndPos = xmlString.find(closeTag, openTagEndPos);

    std::string value(xmlString.substr(openTagEndPos, valueEndPos - openTagEndPos));

    return value;
}

std::string mt::xml::Xml::getXmlDataByTag(std::string_view xmlString, std::string_view tagName, size_t initialPos)
{
    std::string openTag("<");
    openTag += tagName;
    auto startPos = xmlString.find(openTag, initialPos);
//    if (startPos != 0){
//        --startPos;
//    }
    size_t endPos = xmlString.find('>', startPos);
    if (xmlString.at(endPos - 1) == '/'){
        ++endPos;
    }
    else{
        std::string closeTag("</");
        closeTag += tagName;
        closeTag += '>';
        endPos = xmlString.find(closeTag, startPos) + closeTag.length();
    }

    return std::string(xmlString.substr(startPos, endPos - startPos));
}

std::ostream &mt::xml::operator<<(std::ostream &output, const mt::xml::XmlElement &xmlElement)
{
    output << xmlElement.toString_b();

    return output;
}

std::stringstream &mt::xml::operator<<(std::stringstream &output, const mt::xml::XmlElement &xmlElement)
{
    output << xmlElement.toString_b();

    return output;
}

std::ostream &mt::xml::operator<<(std::ostream &output, const mt::xml::Xml &xmlElement)
{
    output << xmlElement.toString();

    return output;
}

std::stringstream &mt::xml::operator<<(std::stringstream &output, const mt::xml::Xml &xmlElement)
{
    output << xmlElement.toString();

    return output;
}
