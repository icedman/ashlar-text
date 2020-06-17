#include <QTextDocument>
#include <QDebug>

#include <iostream>

#include "highlighter.h"
#include "parse.h"
#include "reader.h"
#include "settings.h"

Highlighter::Highlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
    , theme(0)
    , grammar(0)
    , deferRendering(false)
{
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(onUpdate()));
    updateTimer.start(100);
}

void Highlighter::setTheme(theme_ptr _theme)
{
    theme = _theme;
    theme_color(theme, "editor.foreground", foregroundColor);
}

void Highlighter::setLanguage(language_info_ptr _lang)
{
    lang = _lang;
    grammar = _lang->grammar;
}

void Highlighter::setDeferRendering(bool defer)
{
    deferRendering = defer;
}

void Highlighter::setFormatFromStyle(size_t start, size_t length, style_t& style, const char* line, HighlightBlockData* blockData, std::string scope)
{
    // std::cout << scope << std::endl;

    QColor clr;
    if (style.bold == bool_true || style.italic == bool_true || style.underlined == bool_true || style.strikethrough == bool_true || !style.foreground.is_blank()) {
        clr = QColor(style.foreground.red * 255, style.foreground.green * 255, style.foreground.blue * 255, 255);
        QTextCharFormat f;
        f.setFontWeight(style.bold == bool_true ? QFont::Medium : QFont::Normal);
        f.setFontItalic(style.italic == bool_true);
        f.setFontUnderline(style.underlined == bool_true);
        f.setFontStrikeOut(style.strikethrough == bool_true);
        f.setForeground(clr);

        if (scope.find("comment") != std::string::npos) {
            f.setProperty(SCOPE_PROPERTY_ID, SCOPE_COMMENT);
        } else if (scope.find("string") != std::string::npos) {
            f.setProperty(SCOPE_PROPERTY_ID, SCOPE_STRING);
        } else {
            f.setProperty(SCOPE_PROPERTY_ID, SCOPE_OTHER);
        }

        setFormat(start, length, f);
    }

    if (style.foreground.is_blank()) {
        clr = foregroundColor;
    }

    // for minimap
    int s = -1;
    for (int i = start; i < start + length; i++) {
        if (s == -1) {
            if (line[i] != ' ' && line[i] != '\t') {
                s = i;
            }
            continue;
        }
        if (line[i] == ' ' || i + 1 == start + length) {
            if (s != -1) {
                span_info_t span = {
                    .start = s,
                    .length = i - s + 1,
                    .red = clr.red(),
                    .green = clr.green(),
                    .blue = clr.blue()
                };
                blockData->spans.push_back(span);
            }
            s = -1;
        }
    }
}

void Highlighter::highlightBlock(const QString& text)
{
    if (!theme || !grammar) {
        return;
    }

    // std::cout << "highlightBlock" << std::endl;
    
    bool firstLine = true;
    parse::stack_ptr parser_state = NULL;

    //----------------------
    // gather block data
    //----------------------
    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(currentBlock().userData());
    if (!blockData) {
        if (deferRendering) {
            return;
        }
        blockData = new HighlightBlockData;
    }

    std::map<size_t, scope::scope_t> scopes;
     // = blockData->scopes;
    // scopes.clear();
    
    QTextBlock prevBlock = currentBlock().previous();
    HighlightBlockData* prevBlockData = reinterpret_cast<HighlightBlockData*>(prevBlock.userData());
    if (prevBlockData) {
        parser_state = prevBlockData->parser_state;
        if (parser_state->rule) {
            blockData->last_prev_block_rule = parser_state->rule->rule_id;
        }
        firstLine = !(parser_state != NULL);
    }

    if (!parser_state) {
        parser_state = grammar->seed();
        firstLine = true;
    }

    //----------------------
    // parse the line
    //----------------------
    std::string str = text.toStdString();
    // str.assign(text.toUtf8().constData(), text.length());
    str += "\n";

    const char* first = str.c_str();
    const char* last = first + text.length() + 1;

    // std::cout << str << "<<<<" << std::endl;

    if (text.length() > 500) {
        // that would be too long to parse (unminify first)
    } else {
        parser_state = parse::parse(first, last, parser_state, scopes, firstLine);;
    }

    blockData->spans.clear();

    std::string prevScopeName;
    size_t si = 0;
    size_t n = 0;
    std::map<size_t, scope::scope_t>::iterator it = scopes.begin();
    while (it != scopes.end()) {
        n = it->first;
        scope::scope_t scope = it->second;
        std::string scopeName = to_s(scope);
        it++;

        if (n > si) {
            style_t s = theme->styles_for_scope(prevScopeName);
            setFormatFromStyle(si, n - si, s, first, blockData, prevScopeName);
        }

        prevScopeName = scopeName;
        si = n;
    }

    n = last - first;
    if (n > si) {
        style_t s = theme->styles_for_scope(prevScopeName);
        setFormatFromStyle(si, n - si, s, first, blockData, prevScopeName);
    }

    //----------------------
    // langauge config
    //----------------------

    //----------------------
    // find block comments
    //----------------------
    QTextCharFormat format;
    if (lang->blockCommentStart.length()) {
        int beginComment = text.indexOf(lang->blockCommentStart.c_str());
        int endComment = text.indexOf(lang->blockCommentEnd.c_str());
        style_t s = theme->styles_for_scope("comment");

        if (beginComment != -1) {
            format = QSyntaxHighlighter::format(beginComment); 
            if (format.intProperty(SCOPE_PROPERTY_ID) != SCOPE_COMMENT) {
                beginComment = -1;
            }
        }

        if (endComment == -1 && (beginComment != -1 || previousBlockState() == BLOCK_STATE_COMMENT)) {
            setCurrentBlockState(BLOCK_STATE_COMMENT);
            int b = beginComment != -1 ? beginComment : 0;
            int e = endComment != -1 ? endComment : (last - first);
            setFormatFromStyle(b, e - b, s, first, blockData, "comment");
        } else {
            setCurrentBlockState(0);
            if (endComment != -1 && previousBlockState() == BLOCK_STATE_COMMENT) {
                setFormatFromStyle(0, endComment + lang->blockCommentEnd.length(), s, first, blockData, "comment");
            }
        }
    }

    //----------------------
    // gather brackets
    //----------------------
    blockData->brackets.clear();
    if (lang->brackets) {
        std::vector<bracket_info_t> brackets;
        for (char* c = (char*)first; c < last;) {
            bool found = false;
        
            format = QSyntaxHighlighter::format(c-first);
            int prop = format.intProperty(SCOPE_PROPERTY_ID);
            if (prop == SCOPE_COMMENT || prop == SCOPE_STRING) {
                c++;
                continue;
            }

            // opening
            int i = 0;
            for (auto b : lang->bracketOpen) {
                if (strstr(c, b.c_str()) == c) { // << strstr .. fails because it is lazy
                    found = true;
                    size_t l = (c - first);
                    brackets.push_back({ .char_idx = l,
                        .bracket = i,
                        .open = true });
                    c += b.length();
                    break;
                }
            }

            if (found) {
                continue;
            }

            // closing
            i = 0;
            for (auto b : lang->bracketClose) {
                if (strstr(c, b.c_str()) == c) {
                    found = true;
                    size_t l = (c - first);
                    brackets.push_back({ .char_idx = l,
                        .bracket = i,
                        .open = false });
                    c += b.length();
                    break;
                }
            }

            if (found) {
                continue;
            }

            c++;
        }

        // bracket pairing
        for (auto b : brackets) {
            if (!b.open && blockData->brackets.size()) {
                auto l = blockData->brackets.back();
                if (l.open && l.bracket == b.bracket) {
                    blockData->brackets.pop_back();
                } else {
                    // std::cout << "error brackets" << std::endl;
                }
                continue;
            }
            blockData->brackets.push_back(b);
        }

        // hack for if-else-
        if (blockData->brackets.size() == 2) {
            if (blockData->brackets[0].open != blockData->brackets[1].open && blockData->brackets[0].bracket == blockData->brackets[1].bracket) {
                blockData->brackets.clear();
            }
        }

        // format brackets with scope
        // style_t s = theme->styles_for_scope("bracket");
        // for (auto b : blockData->brackets) {
        //     setFormatFromStyle(b.char_idx, 1, s, first, blockData, "bracket");
        // }
    }

    blockData->parser_state = parser_state;
    blockData->dirty = false;
    currentBlock().setUserData(blockData);

    //----------------------
    // mark next block for highlight
    // .. if necessary
    //----------------------
    if (parser_state->rule) {
        QTextBlock next = currentBlock().next();
        if (next.isValid()) {
            HighlightBlockData* nextBlockData = reinterpret_cast<HighlightBlockData*>(next.userData());
            if (nextBlockData && parser_state->rule->rule_id != nextBlockData->last_prev_block_rule) {
                nextBlockData->dirty = true;
                hasDirtyBlocks = true;
            }
        }
    }
}

void Highlighter::onUpdate()
{
    if (!hasDirtyBlocks) {
        return;
    }

    int rendered = 0;

    if (!updateIterator.isValid()) {
        QTextDocument* doc = document();
        updateIterator = doc->begin();
    }

    while (updateIterator.isValid() && rendered < 200) {
        HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(updateIterator.userData());
        if (blockData && blockData->dirty) {
            rendered++;
            rehighlightBlock(updateIterator);
        }
        updateIterator = updateIterator.next();
    }

    if (rendered == 0) {
        hasDirtyBlocks = false;
    }
}
