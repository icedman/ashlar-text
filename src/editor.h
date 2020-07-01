#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include <QCompleter>
#include <QFileSystemWatcher>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QWidget>

#include "extension.h"
#include "grammar.h"
#include "highlighter.h"
#include "theme.h"

class MiniMap;
class Gutter;
class TextmateEdit;
class Editor;

struct editor_settings_t {
    bool mini_map;
    bool gutter;
    float font_size;
    int tab_size;
    bool tab_to_spaces;
    bool word_wrap;
    bool auto_indent;
    bool auto_close;
    bool debug_scopes;
    bool smooth_scroll;
    char font[64];
};

typedef std::shared_ptr<editor_settings_t> editor_settings_ptr;

class Editor : public QWidget {
    Q_OBJECT

public:
    Editor(QWidget* parent = 0);
    ~Editor();

    void setupEditor();
    void setTheme(theme_ptr _theme);
    void setLanguage(language_info_ptr _lang);

    bool openFile(const QString& path = QString());
    bool saveFile(const QString& path = QString());
    void newFile(const QString& path = QString());
    void toggleFold(size_t line);

    bool isPreview();
    void setPreview(bool p);

    void invalidateBuffers();
    bool hasUnsavedChanges();

    QString fullPath() { return fileName; }

    QStringList scopesAtCursor(QTextCursor cursor);

    QString fileName;
    TextmateEdit* editor;
    Gutter* gutter;
    MiniMap* mini;
    Highlighter* highlighter;

    QColor backgroundColor;
    QColor selectionBgColor;

    editor_settings_ptr settings;

    theme_ptr theme;
    language_info_ptr lang;
    parse::grammar_ptr grammar;

    bracket_info_t bracketAtCursor(QTextCursor cursor);
    QTextCursor cursorAtBracket(bracket_info_t bracket);
    QTextCursor findLastOpenBracketCursor(QTextBlock block);
    QTextCursor findBracketMatchCursor(bracket_info_t bracket, QTextCursor cursor);

private:
    QTimer savingTimer;
    QTimer updateTimer;
    QScrollBar* vscroll;
    QTextBlock updateIterator;
    QFileSystemWatcher watcher;

    bool dirty;
    bool preview;

private Q_SLOTS:
    void updateScrollBar();
    void updateScrollBar(int i);
    void updateGutter(bool force = false);
    void updateMiniMap(bool force = false);
    void updateRequested(const QRect& rect, int d);
    void makeDirty(bool undoAvailable);

    void fileChanged(const QString& path);
    void cursorPositionChanged();

public slots:
    void highlightBlocks();
};

#endif // EDITOR_WINDOW_H