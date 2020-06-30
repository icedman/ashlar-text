#include <QDebug>
#include <QPainter>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QSvgRenderer>

#include <iostream>
#include <sstream>
#include <string>

#include "icons.h"

#define ICON_WIDTH 16
#define ICON_HEIGHT 16

struct pixmap_wrapper_t {
    QPixmap img;
};
typedef std::shared_ptr<pixmap_wrapper_t> pixmap_wrapper_ptr;

enum icon_type_t { icon_none,
    icon_font,
    icon_svg };

static void SetAttrRecur(QDomElement elem, QString strtagname, QString strattr, QString strattrval)
{
    // if it has the tagname then overwritte desired attribute
    if (elem.tagName().compare(strtagname) == 0)
    {
        elem.setAttribute(strattr, strattrval);
    }
    // loop all children
    for (int i = 0; i < elem.childNodes().count(); i++)
    {
        if (!elem.childNodes().at(i).isElement())
        {
            continue;
        }
        SetAttrRecur(elem.childNodes().at(i).toElement(), strtagname, strattr, strattrval);
    }
}

static QPixmap SVGIcon(QString path, int width, int height, QColor color)
{

    // open svg resource load contents to qbytearray
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QByteArray baData = file.readAll();
    // load svg contents to xml document and edit contents
    QDomDocument doc;
    doc.setContent(baData);
    // recurivelly change color
    SetAttrRecur(doc.documentElement(), "path", "fill", color.name());
    // create svg renderer with edited contents
    QSvgRenderer svgRenderer(doc.toByteArray());
    // create pixmap target (could be a QImage)
    QPixmap pix(width, height);
    pix.fill(Qt::transparent);
    // create painter to act over pixmap
    QPainter pixPainter(&pix);
    // use renderer to render over painter which paints on pixmap
    svgRenderer.render(&pixPainter);
    return pix;

}
    
std::string to_utf8(uint32_t cp)
{
    // https://stackoverflow.com/questions/28534221/c-convert-asii-escaped-unicode-string-into-utf8-string/47734595.

    std::string result;

    int count;
    if (cp <= 0x007F)
        count = 1;
    else if (cp <= 0x07FF)
        count = 2;
    else if (cp <= 0xFFFF)
        count = 3;
    else if (cp <= 0x10FFFF)
        count = 4;
    else
        return result; // or throw an exception

    result.resize(count);

    if (count > 1) {
        for (int i = count - 1; i > 0; --i) {
            result[i] = (char)(0x80 | (cp & 0x3F));
            cp >>= 6;
        }

        for (int i = 0; i < count; ++i)
            cp |= (1 << (7 - i));
    }

    result[0] = (char)cp;

    return result;
}

std::string wstring_convert(std::string str)
{
    std::string::size_type startIdx = 0;
    do {
        startIdx = str.find("x\\", startIdx);
        if (startIdx == std::string::npos)
            break;
        std::string::size_type endIdx = str.length();
        // str.find_first_not_of("0123456789abcdefABCDEF", startIdx+2);
        if (endIdx == std::string::npos)
            break;
        std::string tmpStr = str.substr(startIdx + 2, endIdx - (startIdx + 2));
        std::istringstream iss(tmpStr);

        uint32_t cp;
        if (iss >> std::hex >> cp) {
            std::string utf8 = to_utf8(cp);
            str.replace(startIdx, 2 + tmpStr.length(), utf8);
            startIdx += utf8.length();
        } else
            startIdx += 2;
    } while (true);

    return str;
}

QPixmap icon_for_file(icon_theme_ptr icons, QString filename, QString suffix, std::vector<Extension>& _extensions, QColor color)
{
    if (!icons) {
        return QPixmap();
    }

    std::string _suffix = suffix.toStdString();
    std::string cacheId = _suffix + color.name().toStdString();
    
    static std::map<std::string, pixmap_wrapper_ptr> cache;
    auto it = cache.find(cacheId);
    if (it != cache.end()) {
        // std::cout << "cached icon.." << std::endl;
        return it->second->img;
    }

    Json::Value definitions = icons->definition["iconDefinitions"];

    // std::cout << definitions std::endl;

    pixmap_wrapper_ptr px = std::make_shared<pixmap_wrapper_t>();

    // std::cout << "generate icon.." << std::endl;

    std::string iconName;
    std::string fontCharacter = "x";
    std::string fontColor;

    Json::Value extensions = icons->definition["fileExtensions"];

    if (icons->definition.isMember(filename.toStdString())) {
        iconName = icons->definition[filename.toStdString()].asString();
    }
    
    if (!iconName.length(), extensions.isMember(_suffix)) {
        iconName = extensions[_suffix].asString();
    }

    if (!iconName.length()) {
        Json::Value languageIds = icons->definition["languageIds"];

        std::string _fileName = "x." + _suffix;
        language_info_ptr lang = language_from_file(_fileName.c_str(), _extensions);
        if (lang) {
            if (languageIds.isMember(lang->id)) {
                iconName = languageIds[lang->id].asString();
            }
        }

        if (!iconName.length()) {
            if (languageIds.isMember(_suffix)) {
                iconName = languageIds[_suffix].asString();
            }
        }
    }

    if (!iconName.length()) {
        iconName = icons->definition["file"].asString();
    }
    
    if (!iconName.length()) {
        return QPixmap();
    }

    icon_type_t icon_type = icon_none;

    int w = ICON_WIDTH;
    int h = ICON_HEIGHT;
    QPixmap svg;

    if (definitions.isMember(iconName)) {
        Json::Value def = definitions[iconName];

        if (def.isMember("fontCharacter")) {
            fontCharacter += def["fontCharacter"].asString();
            fontCharacter += "x";
            fontCharacter = wstring_convert(fontCharacter);
            fontColor = def["fontColor"].asString();
            icon_type = icon_font;
            // std::cout << fontCharacter << std::endl;
        }

        // image type
        if (icon_type == icon_none && def.isMember("iconPath")) {
            std::string imagePath = icons->icons_path + "/" + def["iconPath"].asString();
            // QString imagePath = QString(icons->icons_path.c_str()) +
            // def["iconPath"]; std::cout << imagePath << std::endl;

            // svg = QPixmap(imagePath.c_str());
            svg = SVGIcon(imagePath.c_str(), w, h, color);
            icon_type = icon_svg;
        }
    }

    px->img = QPixmap(w, h);
    px->img.fill(Qt::transparent);

    QPainter p(&px->img);
    p.setRenderHint(QPainter::Antialiasing);

    if (icon_type == icon_font && fontCharacter.length()) {
        p.setPen(QColor(fontColor.c_str()));
        p.setFont(icons->font);
        int fh = QFontMetrics(icons->font).height();
        int fw = QFontMetrics(icons->font).width('w');
        p.drawText(2, -2, w, h, Qt::AlignRight, fontCharacter.c_str());
    } else if (icon_type = icon_svg) {
        p.drawPixmap(px->img.rect(), svg, svg.rect());
    } else {
        return QPixmap();
    }

    cache.emplace(cacheId, px);
    return px->img;
}

QPixmap icon_for_folder(icon_theme_ptr icons, QString folder, bool open, std::vector<Extension>& extensions, QColor color)
{
    if (!icons) {
        return QPixmap();
    }

    Json::Value definitions = icons->definition["iconDefinitions"];

    // std::cout << definitions std::endl;

    pixmap_wrapper_ptr px = std::make_shared<pixmap_wrapper_t>();

    std::string iconName;
    std::string fontCharacter = "x";
    std::string fontColor;

    if (!iconName.length()) {
        if (open) {
            iconName = icons->definition["folderExpanded"].asString();
            if (!iconName.length()) {
                iconName = icons->definition["folder"].asString();
            }
        } else {
            iconName = icons->definition["folder"].asString();
        }
    }

    if (!iconName.length()) {
        // iconName = icons->definition["file"].asString();
        return QPixmap();
    }

    static std::map<std::string, pixmap_wrapper_ptr> cache;
    std::string cacheId = iconName + color.name().toStdString();
    
    auto it = cache.find(cacheId);
    if (it != cache.end()) {
        // std::cout << "cached icon.." << std::endl;
        return it->second->img;
    }

    // std::cout << "generate icon.." << std::endl;

    icon_type_t icon_type = icon_none;

    int w = ICON_WIDTH;
    int h = ICON_HEIGHT;
    QPixmap svg;

    if (definitions.isMember(iconName)) {
        Json::Value def = definitions[iconName];
        if (def.isMember("fontCharacter")) {
            fontCharacter += def["fontCharacter"].asString();
            fontCharacter += "x";
            fontCharacter = wstring_convert(fontCharacter);
            fontColor = def["fontColor"].asString();
            icon_type = icon_font;
            std::cout << fontCharacter << std::endl;
        }

        // image type
        if (icon_type == icon_none && def.isMember("iconPath")) {
            std::string imagePath = icons->icons_path + "/" + def["iconPath"].asString();
            // QString imagePath = QString(icons->icons_path.c_str()) +
            // def["iconPath"]; std::cout << imagePath << std::endl;

            // svg = QPixmap(imagePath.c_str());
            svg = SVGIcon(imagePath.c_str(), w, h, color);
            icon_type = icon_svg;
        }
    }

    px->img = QPixmap(w, h);
    px->img.fill(Qt::transparent);

    QPainter p(&px->img);
    p.setRenderHint(QPainter::Antialiasing);

    if (icon_type == icon_font && fontCharacter.length()) {
        p.setPen(QColor(fontColor.c_str()));
        p.setFont(icons->font);
        int fh = QFontMetrics(icons->font).height();
        int fw = QFontMetrics(icons->font).width('w');
        p.drawText(2, -2, w, h, Qt::AlignRight, fontCharacter.c_str());
    } else if (icon_type = icon_svg) {
        p.drawPixmap(px->img.rect(), svg, svg.rect());
    }

    cache.emplace(cacheId, px);
    return px->img;
}