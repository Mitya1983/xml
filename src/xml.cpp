#include "xml.hpp"

#include <assert.h>
#include <fstream>
#include <filesystem>

#if defined (_WIN32) || defined(_WIN64)
#include <cerrno.h>
#endif

namespace {

const std::string xmlHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n";

} // namespace

xml::XmlElement::XmlElement(std::string_view name) :
    m_name(name)
{

}

xml::XmlElement::XmlElement(std::string_view name, std::string_view value) :
    m_name(name),
    m_value(value)
{

}

void xml::XmlElement::addElement(xml::XmlElement &&element)
{
    assert(m_value.empty() && "XmlElement may comrise either value either child XmlElement");
    m_childs.emplace_back(std::make_shared<xml::XmlElement>(std::move(element)));

}

void xml::XmlElement::addAttribute(std::string_view attributeName, std::string_view attributeValue)
{
    assert(!attributeName.empty() && "XmlElement: attribute name can't be empty");
    assert(!attributeValue.empty() && "XmlElement: attribute value can't be empty");
    m_attributes.emplace_back(attributeName, attributeValue);
}

std::shared_ptr<xml::XmlElement> xml::XmlElement::find(const std::string &name)
{
    for (const auto &element : m_childs){
        if (element->name() == name){
            return element;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<xml::XmlElement> > xml::XmlElement::equal_range(const std::string &name)
{
    std::vector<std::shared_ptr<XmlElement>> result;
    for (const auto &element : m_childs){
        if (element->name() == name){
            result.push_back(element);
        }
    }
    return result;
}

std::string xml::XmlElement::toString() const
{
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

std::string xml::XmlElement::toString_b(uint8_t level) const
{
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

xml::XmlElement xml::XmlElement::fromXmlString(std::string_view xmlString)
{
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
                    throw std::runtime_error("zestad::xml::XmlElement zestad::xml::XmlElement::fromXmlString: xmlData is corrupted due to attribute in tag " + tagName);
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

    xml::XmlElement element(tagName);
    for (const auto &attribute : l_attributes){
        element.addAttribute(attribute.first, attribute.second);
    }
    if (!selfClosingTag){
        if (!_hasChilds(xmlString, tagName)){
            element.setValue(xml::Xml::getValueByTag(xmlString, tagName));
        }
        else{
            std::string xmlValue = xml::Xml::getValueByTag(xmlString, tagName);
            for (const auto &l_tagName : element._getOpenTags(xmlValue)){
                std::string childXmlString = xml::Xml::getXmlDataByTag(xmlValue, l_tagName.first, l_tagName.second);
                element.addElement(xml::XmlElement::fromXmlString(childXmlString));
            }
        }
    }
    return element;
}

std::string xml::XmlElement::_createOpenTag(bool beautify) const
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

std::string xml::XmlElement::_createCloseTag(bool beautify) const
{
    std::string returnValue = "</";
    returnValue += m_name;
    returnValue += "/>";
    if (beautify){
        returnValue += "\n";
    }
    return returnValue;
}

std::vector<std::pair<std::string, size_t>> xml::XmlElement::_getOpenTags(std::string_view xmlData)
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

bool xml::XmlElement::_hasChilds(std::string_view xmlData, std::string_view tagName)
{
    if (Xml::getValueByTag(xmlData, tagName).find('<') != std::string::npos){
        return true;
    }

    return false;
}


void xml::XmlElement::setValue(std::string_view value)
{
    assert(m_childs.empty() && "XmlElement may comrise either value either child XmlElement");
    m_value = value;
}

xml::Xml::Xml(xml::XmlElement &&root) :
    m_root(std::move(root))
{

}

xml::Xml::Xml(const std::string &xmlData) :
    m_root("")
{
    auto headerEndPos = xmlData.find("?>") + 2;
    std::string xmlString = xmlData.substr(headerEndPos);

    m_root = xml::XmlElement::fromXmlString(xmlString);
}

xml::Xml::Xml(const std::filesystem::path &xmlFile) :
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

    *this = xml::Xml(xmlData);

    file.close();
}

std::string xml::Xml::toString() const
{
    std::string returnValue = xmlHeader;
    returnValue += m_root.toString_b();

    return returnValue;
}

void xml::Xml::saveToFile(const std::filesystem::path filePath) const
{
    std::ofstream outputFile;
    outputFile.open(filePath, std::ios::trunc);
    if (!outputFile.is_open()){
        //NOTE: Check implementation on windows
        std::string msg = "Could not create xml file" + filePath.string() + ": " + std::error_code(errno, std::system_category()).message();
        throw std::fstream::failure(msg);
    }
    outputFile << toString();
}

std::string xml::Xml::getValueByTag(std::string_view xmlString, std::string_view tagName, size_t initialPos)
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

std::string xml::Xml::getXmlDataByTag(std::string_view xmlString, std::string_view tagName, size_t initialPos)
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
