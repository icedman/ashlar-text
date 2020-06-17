#include <QColor>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QFontDatabase>

#include <iostream>

#include "extension.h"
#include "reader.h"
#include "stringop.h"

#include "json/json.h"

void load_settings(const QString path, Json::Value& settings)
{
    Json::Value obj = parse::loadJson(path.toStdString() + "/settings.json");
    if (!obj.isObject()) {
        return;
    }

    std::vector<std::string> keys = obj.getMemberNames();
    std::vector<std::string>::iterator it = keys.begin();
    while (it != keys.end()) {
        settings[*it] = obj[*it];
        it++;
    }
}

void load_extensions(const QString path, std::vector<Extension>& extensions)
{
    // Json::Value contribs;

    QDirIterator it(path);
    while (it.hasNext()) {
        QString extensionPath = it.next();
        QString package = extensionPath + "/package.json";

        // qDebug() << package;
        Extension ex = {
            .path = extensionPath,
        };

        ex.package = parse::loadJson(package.toStdString());
        if (!ex.package.isObject()) {
            continue;
        }

        bool append = false;
        if (ex.package.isMember("contributes")) {
            std::vector<std::string> keys = ex.package["contributes"].getMemberNames();
            std::vector<std::string>::iterator c_it = keys.begin();
            while (c_it != keys.end()) {
                std::string name = *c_it;

                if (name == "themes" || name == "iconThemes" || name == "languages") {
                    append = true;

                    // Json::Value obj;
                    // obj["name"] = ex.package["name"];
                    // obj["package"] = package.toStdString();
                    // contribs[name].append(obj);

                    break;
                }

                c_it++;
            }
        }

        if (append) {
            // std::cout << ex.package["name"].asString() << std::endl;
            // qDebug() << package;
            extensions.emplace_back(ex);
        }
    }

    // std::cout << contribs;
}

static bool load_language_configuration(const QString path, language_info_ptr lang)
{
    Json::Value root = parse::loadJson(path.toStdString());
    
    if (root.empty()) {
        return false;
    }
    
    if (root.isMember("comments")) {
        Json::Value comments = root["comments"];

        if (comments.isMember("lineComment")) {
            lang->lineComment = comments["lineComment"].asString();
        }

        if (comments.isMember("blockComment")) {
            Json::Value blockComment = comments["blockComment"];
            if (blockComment.isArray() && blockComment.size() == 2) {
                std::string beginComment = comments["blockComment"][0].asString();
                std::string endComment = comments["blockComment"][1].asString();
                if (beginComment.length() && endComment.length()) {
                    lang->blockCommentStart = beginComment;
                    lang->blockCommentEnd = endComment;
                }
            }
        }
    }

    if (root.isMember("brackets")) {
        Json::Value brackets = root["brackets"];
        if (brackets.isArray()) {
            for (int i = 0; i < brackets.size(); i++) {
                Json::Value pair = brackets[i];
                if (pair.isArray() && pair.size() == 2) {
                    if (pair[0].isString() && pair[1].isString()) {
                        lang->bracketOpen.push_back(pair[0].asString());
                        lang->bracketClose.push_back(pair[1].asString());
                    }
                }
            }
            lang->brackets = lang->bracketOpen.size();
        }
    }

    if (root.isMember("autoClosingPairs")) {
        Json::Value pairs = root["autoClosingPairs"];
        if (pairs.isArray()) {
            for (int i = 0; i < pairs.size(); i++) {
                Json::Value pair = pairs[i];
                if (pair.isObject()) {
                    if (pair.isMember("open") && pair.isMember("close")) {
                        lang->pairOpen.push_back(pair["open"].asString());
                        lang->pairClose.push_back(pair["close"].asString());
                    }
                }
            }
            lang->pairs = lang->pairOpen.size();
        }
    }
    
    return true;
}

language_info_ptr language_from_file(const QString path, std::vector<Extension>& extensions)
{
    static std::map<std::string, language_info_ptr> cache;
    language_info_ptr lang = std::make_shared<language_info_t>();

    QFileInfo info(path);
    std::string suffix = ".";
    suffix += info.suffix().toStdString();

    auto it = cache.find(suffix);
    if (it != cache.end()) {
        qDebug() << "langauge matched from cache";
        return it->second;
    }

    // check cache
    Extension resolvedExtension;
    std::string resolvedLanguage;
    Json::Value resolvedGrammars;
    Json::Value resolvedConfiguration;

    for (auto ext : extensions) {
        Json::Value contribs = ext.package["contributes"];
        if (!contribs.isMember("languages") || !contribs.isMember("grammars")) {
            continue;
        }
        Json::Value langs = contribs["languages"];
        for (int i = 0; i < langs.size(); i++) {
            Json::Value lang = langs[i];
            if (!lang.isMember("extensions") || !lang.isMember("id")) {
                continue;
            }

            Json::Value exts = lang["extensions"];
            for (int j = 0; j < exts.size(); j++) {
                Json::Value ex = exts[j];
                // if (ex.asString().compare(suffix) == 0) {
                if (ex.asString() == suffix) {
                    resolvedExtension = ext;
                    resolvedLanguage = lang["id"].asString();
                    resolvedGrammars = contribs["grammars"];
                    
                    qDebug() << resolvedLanguage.c_str();
                    
                    break;
                }
            }

            if (!resolvedLanguage.empty()) {
                if (lang.isMember("configuration")) {
                    resolvedConfiguration = lang["configuration"];
                }
                break;
            }
        }

        if (!resolvedLanguage.empty())
            break;
    }

    if (!resolvedLanguage.empty()) {
        // std::cout << resolvedLanguage << std::endl;
        for (int i = 0; i < resolvedGrammars.size(); i++) {
            Json::Value g = resolvedGrammars[i];
            if (!g.isMember("language")) {
                continue;
            }

            if (g["language"].asString().compare(resolvedLanguage) == 0) {
                QString path = QDir(resolvedExtension.path).filePath(g["path"].asString().c_str());
                lang->grammar = parse::parse_grammar(parse::loadJson(path.toStdString()));
                lang->id = resolvedLanguage;

                // language configuration
                if (!resolvedConfiguration.empty()) {
                    path = QDir(resolvedExtension.path).filePath(resolvedConfiguration.asString().c_str());
                } else {
                    path = QDir(resolvedExtension.path).filePath("language-configuration.json");
                }
                load_language_configuration(path, lang);

                qDebug() << "langauge matched";
                qDebug() << path;

                // cache.emplace(suffix, lang);
                // return lang;
            }
        }
    }

    if (!lang->grammar) {
        Json::Value empty;
        empty["scopeName"] = suffix;
        lang->id = suffix;
        lang->grammar = parse::parse_grammar(empty);
    }

    cache.emplace(suffix, lang);
    return lang;
}

icon_theme_ptr icon_theme_from_name(const QString path, std::vector<Extension>& extensions)
{
    icon_theme_ptr icons = std::make_shared<icon_theme_t>();

    std::string theme_path = path.toStdString();
    std::string icons_path;
    bool found = false;
    for (auto ext : extensions) {
        Json::Value contribs = ext.package["contributes"];
        if (!contribs.isMember("iconThemes")) {
            continue;
        }

        Json::Value themes = contribs["iconThemes"];
        for (int i = 0; i < themes.size(); i++) {
            Json::Value theme = themes[i];
            if (theme["id"].asString() == theme_path || theme["label"].asString() == theme_path) {
                theme_path = ext.path.toStdString() + "/" + theme["path"].asString();
                // icons_path = ext.path.toStdString() + "/icons/";
                icons_path = QFileInfo(QString(theme_path.c_str())).path().toStdString() + "/";
                found = true;
                break;
            }
        }

        if (found) {
            break;
        }
    }

    if (!found) {
        return icons;
    }

    Json::Value json = parse::loadJson(theme_path);
    icons->icons_path = icons_path;

    if (json.isMember("fonts")) {
        Json::Value fonts = json["fonts"];
        Json::Value font = fonts[0];
        Json::Value family = font["id"];
        Json::Value src = font["src"][0];
        Json::Value src_path = src["path"];
        std::string real_font_path = icons_path + src_path.asString();
        QFontDatabase::addApplicationFont(real_font_path.c_str());

        // icons->font.setFamily("monospace");
        icons->font.setFamily(family.asString().c_str());
        icons->font.setPointSize(16);
        icons->font.setFixedPitch(true);
    }

    // todo implement svg icons

    icons->definition = json;
    return icons;
}

theme_ptr theme_from_name(const QString path, std::vector<Extension>& extensions)
{
    std::string theme_path = path.toStdString();
    bool found = false;
    for (auto ext : extensions) {
        Json::Value contribs = ext.package["contributes"];
        if (!contribs.isMember("themes")) {
            continue;
        }

        Json::Value themes = contribs["themes"];
        for (int i = 0; i < themes.size(); i++) {
            Json::Value theme = themes[i];
            if (theme["id"].asString() == theme_path || theme["label"].asString() == theme_path) {
                theme_path = ext.path.toStdString() + "/" + theme["path"].asString();
                std::cout << ext.path.toStdString() << "..." << std::endl;
                std::cout << theme_path << std::endl;
                found = true;
                break;
            }
        }

        if (found) {
            break;
        }
    }

    Json::Value json = parse::loadJson(theme_path);
    theme_ptr theme = parse_theme(json);
    return theme;
}