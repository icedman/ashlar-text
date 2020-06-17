#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTimer>

#include "extension.h"
#include "grammar.h"
#include "theme.h"

struct block_info_t {
    int position;
    int number;
    bool foldable : 1;
};

struct span_info_t {
    int start;
    int length;
    int red;
    int green;
    int blue;
};

struct bracket_info_t {
    size_t line;
    size_t char_idx;
    int bracket;
    bool open;
    bool unpaired;
};

#define SCOPE_PROPERTY_ID 0x99

enum {
    SCOPE_UNSET     = 0,
    SCOPE_STRING    = 1,
    SCOPE_COMMENT   = 2,
    SCOPE_OTHER     = 3,
};

enum {
    BLOCK_STATE_COMMENT         = 1 << 1,
    BLOCK_STATE_BLOCK           = 1 << 2,
    BLOCK_STATE_BLOCK_NESTED    = 1 << 3
};

class HighlightBlockData : public QTextBlockUserData {
public:
    HighlightBlockData()
        : QTextBlockUserData()
        , dirty(false)
        , folded(false)
        , last_prev_block_rule(0)
    {
    }

    parse::stack_ptr parser_state;
    scope::scope_t last_scope;
    bool dirty;
    bool folded;
    size_t last_prev_block_rule;

    std::vector<span_info_t> spans;
    std::vector<bracket_info_t> brackets;
    std::map<size_t, scope::scope_t> scopes;
};

class Highlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    Highlighter(QTextDocument* parent = 0);

    void setTheme(theme_ptr theme);
    void setLanguage(language_info_ptr lang);
    void setDeferRendering(bool defer);

    bool isDirty() { return hasDirtyBlocks; }
    bool isReady() { return !deferRendering; }

protected:
    void highlightBlock(const QString& text) override;
    void setFormatFromStyle(size_t start, size_t length, style_t& style, const char* line, HighlightBlockData* blockData, std::string scope);

private:
    bool deferRendering;

    language_info_ptr lang;
    parse::grammar_ptr grammar;
    theme_ptr theme;

    QColor foregroundColor;

    bool hasDirtyBlocks;
    QTextBlock updateIterator;
    QTimer updateTimer;

private Q_SLOTS:
    void onUpdate();
};

#endif // HIGHLIGHTER_H