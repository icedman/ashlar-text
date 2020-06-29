#include <QtWidgets>

#include <iostream>

#include "commands.h"
#include "editor.h"
#include "gutter.h"
#include "mainwindow.h"
#include "minimap.h"
#include "reader.h"
#include "settings.h"
#include "tmedit.h"

Editor::Editor(QWidget* parent)
    : QWidget(parent)
    , theme(0)
    , grammar(0)
    , gutter(0)
    , mini(0)
    , highlighter(0)
    , editor(0)
    , savingTimer(this)
    , updateTimer(this)
    , dirty(false)
{
    savingTimer.setSingleShot(true);
    connect(&watcher, SIGNAL(fileChanged(const QString&)), this, SLOT(fileChanged(const QString&)));
}

Editor::~Editor()
{
}

void Editor::newFile(const QString& path)
{
    editor->clear();
    fileName = path;
}

bool Editor::saveFile(const QString& path)
{
    QFile file(path);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        savingTimer.start(2000);
        QTextStream out(&file);
        out << editor->toPlainText();

        fileName = path;
        watcher.removePaths(watcher.files());
        watcher.addPath(fileName);
        dirty = false;
        return true;
    }
    return false;
}

bool Editor::openFile(const QString& path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        fileName = path;
        highlighter->setLanguage(lang);

        if (watcher.files().size()) {
            watcher.removePaths(watcher.files());
        }
        watcher.addPath(fileName);
        dirty = false;

        if (file.size() > (1024 * 16)) {
            // todo do super load!
            if (file.size() > (1024 * 256)) {
                std::cout << "do threaded syntax highlighting at load" << std::endl;
            }

            std::cout << file.size() << std::endl;
            highlighter->setDeferRendering(true);
            editor->setPlainText(file.readAll());
            highlightBlocks();
        } else {
            highlighter->setDeferRendering(false);
            editor->setPlainText(file.readAll());
        }

        return true;
    }
    return false;
}

void Editor::invalidateBuffers()
{
    QTextBlock block = editor->document()->begin();
    while (block.isValid()) {
        HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
        if (blockData) {
            blockData->buffer = QPixmap();
        }
        block = block.next();
    }
}

void Editor::makeDirty(bool undoAvailable)
{
    dirty = undoAvailable;
}

void Editor::fileChanged(const QString& path)
{
    if (savingTimer.isActive()) {
        return;
    }

    // todo .. if has undo.. prompt
    qDebug() << "file changed, reloading...";

    QTextCursor tc = editor->textCursor();
    if (openFile(fileName)) {
        editor->setTextCursor(tc);
    }
}

void Editor::cursorPositionChanged()
{
    MainWindow::instance()->emitEvent("cursorPositionChanged", "");
}

bool Editor::hasUnsavedChanges() {
    return dirty;
}

void Editor::setTheme(theme_ptr _theme)
{
    theme = _theme;

    if (!editor) {
        return;
    }

    if (highlighter) {
        highlighter->setTheme(theme);
    }

    //------------------
    // editor theme
    //------------------
    bool isDark = theme_is_dark(theme);
    QColor bgColor;
    QColor fgColor;
    QColor lineNumberColor;

    if (theme_color(theme, "editor.foreground", fgColor)) {
        QTextCharFormat fmt;
        fmt.setForeground(QBrush(fgColor));
        editor->mergeCurrentCharFormat(fmt);
        gutter->lineNumberColor = fgColor;
    }

    if (theme_color(theme, "editorLineNumber.foreground", lineNumberColor)) {
        gutter->lineNumberColor = lineNumberColor;
    }

    if (theme_color(theme, "editor.background", bgColor)) {
        QPalette p = editor->palette();

        p.setColor(QPalette::Active, QPalette::Base, bgColor);
        p.setColor(QPalette::Inactive, QPalette::Base, bgColor);
        p.setColor(QPalette::HighlightedText, fgColor);

        editor->setPalette(p);

        gutter->backgroundColor = bgColor;
        mini->backgroundColor = bgColor;
    }

    if (theme_color(theme, "editor.selectionBackground", selectionBgColor)) {
        if (isDark) {
            selectionBgColor = selectionBgColor.lighter(110);
        } else {
            selectionBgColor = selectionBgColor.darker(110);
        }
    }

    backgroundColor = bgColor;

    editor->setStyleSheet("QPlainTextEdit { border: 0px; } QScrollBar:vertical { width: 0px }");

    if (!settings->word_wrap) {
        editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    }

    if (highlighter) {
        highlighter->rehighlight();
    }

    updateGutter(true);
    updateMiniMap(true);
}

void Editor::setLanguage(language_info_ptr _lang)
{
    lang = _lang;
    grammar = _lang->grammar;
    if (highlighter) {
        highlighter->setLanguage(lang);
    }
}

void Editor::setupEditor()
{
    if (editor != 0) {
        // require a theme
        // no reinit if already done
        return;
    }

    QFont font;
    font.setFamily(settings->font.c_str());
    font.setPointSize(settings->font_size);
    font.setFixedPitch(true);

    editor = new TextmateEdit(this);
    editor->setFont(font);
    editor->setTabStopDistance(QFontMetrics(font).horizontalAdvance('w') * settings->tab_size);

    connect(editor, SIGNAL(blockCountChanged(int)), this, SLOT(updateGutter()));
    connect(editor, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateRequested(QRect, int)));
    connect(editor, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
    connect(editor, SIGNAL(undoAvailable(bool)), this, SLOT(makeDirty(bool)));

    gutter = new Gutter(this);
    gutter->font = font;
    gutter->editor = this;

    mini = new MiniMap(this);
    mini->editor = this;

    vscroll = new QScrollBar(this);

    connect(editor->verticalScrollBar(), SIGNAL(valueChanged(int)), vscroll, SLOT(setValue(int)));
    connect(vscroll, SIGNAL(valueChanged(int)), editor->verticalScrollBar(), SLOT(setValue(int)));
    connect(mini, SIGNAL(valueChanged(int)), editor->verticalScrollBar(), SLOT(setValue(int)));

    // updates everyone
    connect(vscroll, SIGNAL(valueChanged(int)), this, SLOT(updateScrollBar(int)));
    connect(editor->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateScrollBar(int)));

    QHBoxLayout* box = new QHBoxLayout(this);

    box->setMargin(0);
    box->setSpacing(0);

    box->addWidget(gutter);
    box->addWidget(editor);
    box->addWidget(mini);
    box->addWidget(vscroll);

    setLayout(box);

    // gutter->hide();
    // mini->hide();

    if (theme) {
        setTheme(theme);
    }

    //------------------
    // setup highlighter
    //------------------
    highlighter = new Highlighter(editor->document());
    highlighter->setTheme(theme);

    updateMiniMap();
}

void Editor::updateRequested(const QRect& rect, int d)
{
    static QRect previousRect;

    if (previousRect == rect) {
        // probably just the cursor pulse
        return;
    }

    updateGutter();
    updateMiniMap();

    previousRect = rect;
}

void Editor::updateMiniMap(bool force)
{
    if (!mini) {
        return;
    }

    if (settings->mini_map) {
        mini->show();
    } else {
        mini->hide();
        return;
    }

    int sw = 60 + (width() * 0.03);
    mini->setMinimumSize(sw, 0);
    mini->update();

    int first = 0;
    if (gutter->lineNumbers.size()) {
        first = gutter->lineNumbers[0].number;
    }

    if (force) {
        mini->setSizes(0, 0, 0, 0);
    }
    mini->setSizes(first, gutter->lineNumbers.size(), vscroll->value(), vscroll->maximum());
}

void Editor::updateScrollBar()
{
    QScrollBar* editorScroll = editor->verticalScrollBar();
    size_t max = editorScroll->maximum();
    if (max > 0) {
        vscroll->show();
    } else {
        vscroll->hide();
    }

    vscroll->setMaximum(max);
    vscroll->setSingleStep(editorScroll->singleStep());
    vscroll->setPageStep(editorScroll->pageStep());

    // yep-minimap is a scrollbar
    mini->setMaximum(max);
    mini->setSingleStep(editorScroll->singleStep());
    mini->setPageStep(editorScroll->pageStep());

    updateMiniMap();

    editor->paintToBuffer();
}

void Editor::updateScrollBar(int i)
{
    updateGutter();
    updateScrollBar();
}

static bool isFoldable(QTextBlock& block)
{
    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    if (blockData) {
        return blockData->foldable;
    }
    return false;
}

void Editor::updateGutter(bool force)
{
    if (!gutter) {
        return;
    }

    if (settings->gutter) {
        gutter->show();
    } else {
        gutter->hide();
        return;
    }

    int sw = 0;
    int digits = 2 + 2;
    int maxLines = editor->blockCount();
    for (int number = 10; number < maxLines; number *= 10)
        ++digits;
    sw += editor->fontMetrics().width('w') * digits;

    gutter->setMinimumSize(sw, height());
    QRectF sidebarRect(0, 0, sw, height());

    QTextBlock block = editor->_firstVisibleBlock();
    if (block.previous().isValid()) {
        block = block.previous();
    }

    int index = 0;
    while (block.isValid()) {
        if (block.isVisible()) {
            QRectF rect = editor->_blockBoundingGeometry(block).translated(editor->_contentOffset());
            // if (sidebarRect.intersects(rect)) {
            if (gutter->lineNumbers.count() >= index)
                gutter->lineNumbers.resize(index + 1);
            gutter->lineNumbers[index].position = rect.top();
            gutter->lineNumbers[index].number = block.blockNumber() + 1;
            gutter->lineNumbers[index].foldable = isFoldable(block);
            ++index;
            // }
            if (rect.top() > sidebarRect.bottom() + 40)
                break;
        }
        block = block.next();
    }
    gutter->lineNumbers.resize(index);
    gutter->update();
    updateScrollBar();
}

void Editor::highlightBlocks()
{
    int rendered = 0;

    if (!updateIterator.isValid()) {
        QTextDocument* doc = editor->document();
        updateIterator = doc->begin();
    }

    while (updateIterator.isValid() && rendered < 200) {
        HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(updateIterator.userData());
        if (!blockData) {
            rendered++;
            blockData = new HighlightBlockData;
            updateIterator.setUserData(blockData);
            highlighter->rehighlightBlock(updateIterator);
        }
        updateIterator = updateIterator.next();
    }

    if (rendered > 0) {
        updateTimer.singleShot(50, this, SLOT(highlightBlocks()));
    } else {
        std::cout << "all rendering done" << std::endl;
        highlighter->setDeferRendering(false);
    }

    editor->paintToBuffer();
    mini->buffer = QPixmap();
    mini->update();
}

bracket_info_t Editor::bracketAtCursor(QTextCursor cursor)
{
    bracket_info_t b;
    b.line = -1;
    b.position = -1;
    
    QTextBlock block = cursor.block();
    if (!block.isValid()) {
        return b;
    }
    
    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    if (!blockData) {
        return b;
    }
    
    size_t p = cursor.position() - block.position();
    for(auto bracket : blockData->brackets) {
        if (bracket.position == p) {
            return bracket;
        }
    }
        
    return b;
}

QTextCursor Editor::cursorAtBracket(bracket_info_t bracket)
{
    QTextCursor cursor;
    
    QTextBlock block = editor->textCursor().block();
    while(block.isValid()) {
        if (block.firstLineNumber() == bracket.line) {
            cursor = editor->textCursor();
            cursor.setPosition(block.position() + bracket.position);
            break;
        }
        if (block.firstLineNumber() > bracket.line) {
            block = block.next();
        } else {
            block = block.previous();
        }
    }    
    return cursor;
}

QTextCursor Editor::findLastOpenBracketCursor(QTextBlock block)
{
    if (!block.isValid()) {
        return QTextCursor();
    }
    
    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    if (!blockData) {
        return QTextCursor();
    }
    
    QTextCursor res;    
    for(auto b : blockData->foldingBrackets) {
        if (b.open) {
            if (res.isNull()) {
                res = editor->textCursor();
            }
            res.setPosition(block.position() + b.position);
        }
    }
    
    return res;
}

QTextCursor Editor::findBracketMatchCursor(bracket_info_t bracket, QTextCursor cursor)
{
    QTextCursor cs(cursor);
    
    std::vector<bracket_info_t> brackets;        
    QTextBlock block = cursor.block();
        
    if (bracket.open) {
            
        while(block.isValid()) {
            HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
            if (!blockData) {
                break;
            }
            
            for(auto b : blockData->brackets) {
                if (b.line == bracket.line && b.position < bracket.position) {
                    continue;
                }
                
                if (!b.open) {
                    auto l = brackets.back();
                    if (l.open && l.bracket == b.bracket) {
                        brackets.pop_back();
                    } else {
                        // error .. unpaired?
                        return QTextCursor();
                    }
    
                    if (!brackets.size()) {
                        // std::cout << "found end!" << std::endl;
                        cursor.setPosition(block.position() + b.position);
                        return cursor;
                    }
                    continue;
                }
                brackets.push_back(b);
            }
            
            block = block.next();
        }
    
    } else {
    
        // reverse
        while(block.isValid()) {
            HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
            if (!blockData) {
                break;
            }
            
            // for(auto b : blockData->brackets) {
            for (auto it =  blockData->brackets.rbegin(); it != blockData->brackets.rend(); ++it) {
                bracket_info_t b = *it;
                if (b.line == bracket.line && b.position > bracket.position) {
                    continue;
                }
                
                if (b.open) {
                    auto l = brackets.back();
                    if (!l.open && l.bracket == b.bracket) {
                        brackets.pop_back();
                    } else {
                        // error .. unpaired?
                        return QTextCursor();
                    }
    
                    if (!brackets.size()) {
                        // std::cout << "found begin!" << std::endl;
                        cursor.setPosition(block.position() + b.position);
                        return cursor;
                    }
                    continue;
                }
                brackets.push_back(b);
            }
            
            block = block.previous();
        }
        
    }
    
    return QTextCursor();
}

void Editor::toggleFold(size_t line)
{
    QTextDocument* doc = editor->document();
    QTextBlock folder = doc->findBlockByNumber(line - 1);
    
    QTextCursor openBracket = findLastOpenBracketCursor(folder);
    if (openBracket.isNull()) {
        return;
    }
    
    bracket_info_t bracket = bracketAtCursor(openBracket);
    if (bracket.line == -1 || bracket.position == -1) {
        return;
    }
    QTextCursor endBracket = findBracketMatchCursor(bracket, openBracket);
    if (endBracket.isNull()) {
        return;
    }
    
    QTextBlock block = openBracket.block();
    QTextBlock endBlock = endBracket.block();
    
    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    if (!blockData) {
        return;
    }
    
    blockData->folded = !blockData->folded;
    block = block.next();
    while(block.isValid()) {
        HighlightBlockData* targetData = reinterpret_cast<HighlightBlockData*>(block.userData());
        targetData->folded = blockData->folded;
        if (blockData->folded) {
            block.setVisible(false);
            block.setLineCount(0);
        } else {
            block.setVisible(true);
            block.setLineCount(1);
        }
        if (block == endBlock) {
            break;
        }
        block = block.next();
    }

    editor->update();
    updateGutter();
}

QStringList Editor::scopesAtCursor(QTextCursor cursor)
{
    QStringList res;
    QTextBlock block = cursor.block();
    if (!block.isValid()) {
        return res;
    }
    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    if (!blockData) {
        return res;
    }

    size_t pos = cursor.position();
    cursor.movePosition(QTextCursor::StartOfLine);
    pos -= cursor.position();

    std::map<size_t, scope::scope_t> scopes = blockData->scopes;
    std::map<size_t, scope::scope_t>::iterator it = scopes.begin();
    scope::scope_t scope;
    while (it != scopes.end()) {
        size_t n = it->first;
        if (n > pos) {
            break;
        }
        scope = it->second;
        it++;
    }

    std::string scopeName = to_s(scope);
    res << QString(scopeName.c_str()).split(' ');
    return res;
}
