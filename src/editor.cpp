#include <QtWidgets>
#include <iostream>

#include "commands.h"
#include "editor.h"
#include "gutter.h"
#include "mainwindow.h"
#include "minimap.h"
#include "reader.h"
#include "settings.h"

#define SMOOTH_SCROLL_THRESHOLD_X 400
#define SMOOTH_SCROLL_THRESHOLD_Y 400
#define SMOOTH_SCROLL_FRAMES 2
#define SMOOTH_SCROLL_FRICTION_X 0.9
#define SMOOTH_SCROLL_FRICTION_Y 0.8
#define SMOOTH_SCROLL_X 8

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
{
    savingTimer.setSingleShot(true);
    connect(&watcher, SIGNAL(fileChanged(const QString&)), this, SLOT(fileChanged(const QString&)));
}

Editor::~Editor()
{
    qDebug() << "free editor";
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
    
    if (!bracket.open) {
        return cs;
    }
    
    std::vector<bracket_info_t> brackets;
    
    QTextBlock block = cursor.block();
    while(block.isValid()) {
        HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
        if (!blockData) {
            break;
        }
        
        for(auto b : blockData->brackets) {
            if (b.line == bracket.line && b.position <bracket.position) {
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

//---------------------
// overlay
//---------------------
Overlay::Overlay(QWidget* parent)
    : QWidget(parent)
    , updateTimer(this)
    , cursorOn(true)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateCursor()));
    updateTimer.start(750);
}

void Overlay::updateCursor()
{

    TextmateEdit* editor = qobject_cast<TextmateEdit*>(QApplication::focusWidget());
    if (!editor) {
        cursorOn = false;
        return;
    }

    cursorOn = !cursorOn;
    update();
}

void Overlay::paintEvent(QPaintEvent*)
{
    // this actually draws the QPlainTextEdit widget .. with some extras
    QWidget* container = (QWidget*)parent();
    resize(container->width(), container->height());

    QPainter p(this);
    p.drawPixmap(rect(), buffer, buffer.rect());
    
    if (!cursorOn) {
        return;
    }

    TextmateEdit* editor = (TextmateEdit*)parent();
    Editor* e = (Editor*)editor->parent();
    
    p.translate(0, editor->offset().y());
    
    QList<QTextCursor> cursors;
    cursors << editor->extraCursors;
    cursors << editor->textCursor();
        
    QTextBlock block = editor->_firstVisibleBlock();
    while (block.isValid()) {
        QRectF rect = editor->_blockBoundingGeometry(block).translated(editor->_contentOffset());
        if (rect.top() > height())
            break;

        if (block.isVisible()) {

            QTextLayout* layout = block.layout();

            //-----------------
            // cursors
            //-----------------
            if (cursorOn) {
                for (auto cursor : cursors) {
                    if (cursor.block() == block) {
                        layout->drawCursor(&p, rect.topLeft(), cursor.position() - block.position());
                    }
                }
            }
        }
        block = block.next();
    }
}

void Overlay::mousePressEvent(QMouseEvent* event)
{
    // std::cout << "still got to me" << std::endl;
    // listening to click events
}

//---------------------
// custom QPlainTextEdit
//---------------------
TextmateEdit::TextmateEdit(QWidget* parent)
    : QPlainTextEdit(parent)
    , updateTimer(this)
    , _offset(QPointF(0, 0))
{
    overlay = new Overlay(this);
    editor = (Editor*)parent;

    completer = new QCompleter(this);
    completer->setModel(new QStringListModel());
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(true);
    completer->setWidget(this);

    QObject::connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
        this, &TextmateEdit::insertCompletion);

    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateScrollDelta()));
}

void TextmateEdit::contextMenuEvent(QContextMenuEvent* event)
{
    // QMenu *menu = createStandardContextMenu();
    // menu->addAction(tr("My Menu Item"));
    // //...
    // menu->exec(event->globalPos());
    // delete menu;
}

void TextmateEdit::paintToBuffer()
{
    // paint to buffer
    QPixmap map(width(), height());
    // map.fill(Qt::transparent);

    QPainter p(&map);
    if (scrollVelocity.x() == 0 && scrollVelocity.y() == 0)
        p.setRenderHint(QPainter::Antialiasing);

    TextmateEdit* editor = this;
    Editor* e = (Editor*)editor->parent();
    p.fillRect(rect(), e->backgroundColor);

    QColor selectionBg = e->selectionBgColor;
    QColor foldedBg = selectionBg;
    foldedBg.setAlpha(64);

    QList<QTextCursor> cursors;
    cursors << extraCursors;
    cursors << textCursor();

    QTextBlock block = editor->_firstVisibleBlock();
    QFontMetrics fm(font());
    float fh = fm.height();

    QTextLayout* layout = block.layout();
    if (layout) {
        QTextLine line = layout->lineAt(0);
        if (line.isValid()) {
            fh = layout->lineAt(0).height();
        }
    }

    float x = 0;
    float y = fh * scrollDelta.y() / SMOOTH_SCROLL_THRESHOLD_Y;
    _offset = QPointF(x, y);
    p.translate(x, y);

    if (x != 0 && y != 0) {
        overlay->cursorOn = false;
    }

    QList<QTextCursor> pairs;
    
    // bracket pairing
    bracket_info_t bracket = e->bracketAtCursor(editor->textCursor());
    if (bracket.line != -1 && bracket.position != -1) {
        QTextCursor start = editor->textCursor();
        start.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        pairs.push_back(start);
        cursors << start;
        
        QTextCursor end = e->findBracketMatchCursor(bracket, editor->textCursor());
        end.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        pairs.push_back(end);
        cursors << end;
    }
    
    //-----------------
    // selections
    //-----------------
    for (auto cursor : cursors) {
        if (!cursor.hasSelection()) {
            continue;
        }

        QTextCursor cs(cursor);
        cs.setPosition(cs.selectionStart());

        while (cs.position() < cursor.selectionEnd()) {
            QTextBlock block = cs.block();
            QRectF r = editor->_blockBoundingGeometry(block).translated(editor->_contentOffset());
            if (r.top() > height() + 20) {
                break;
            }

            if (block.isVisible()) {
                QTextLayout* layout = block.layout();

                for (int i = 0; i < layout->lineCount(); i++) {
                    QTextLine line = layout->lineAt(i);

                    int sx = line.textStart();
                    if (sx + block.position() < cursor.selectionStart()) {
                        sx = cursor.selectionStart() - block.position();
                    }

                    int ex = sx + line.textLength();
                    if (ex + block.position() > cursor.selectionEnd()) {
                        ex = cursor.selectionEnd() - block.position();
                    }

                    qreal srx = line.cursorToX(&sx);
                    qreal erx = line.cursorToX(&ex);
                    float w = erx - srx;
                    float h = fh;
                    float offsetY = 0;
                    
                    if (pairs.contains(cursor)) {
                        offsetY = h - 1;
                        h = 1;
                    }

                    r.setWidth(w);
                    p.fillRect(QRect(r.left() + srx, r.top() + (i * fh) + offsetY, w, h), e->selectionBgColor);
                }
            }

            if (!cs.movePosition(QTextCursor::Down)) {
                break;
            }
            cs.movePosition(QTextCursor::StartOfLine);
        }
    }

    block = firstVisibleBlock();
    if (block.previous().isValid()) {
        block = block.previous();
    }
    while (block.isValid()) {
        QRectF r = blockBoundingGeometry(block).translated(contentOffset());
        if (r.top() > height() + 20) {
            break;
        }

        HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
        if (blockData && block.isVisible()) {
            QTextLayout* layout = block.layout();

            //-----------------
            // folded indicator
            //-----------------
            if (blockData->folded) {
                p.fillRect(r, foldedBg);
            }

            //-----------------
            // render the block
            //-----------------
            if (e->settings->smooth_scroll) {
                if (blockData->buffer.width() != r.width() || blockData->buffer.height() != r.height()) {
                    blockData->buffer = QPixmap(r.width(), r.height());
                    blockData->buffer.fill(Qt::transparent);
                    QPainter pp(&blockData->buffer);
                    pp.setRenderHint(QPainter::Antialiasing);
                    layout->draw(&pp, QPointF(0, 0), QVector<QTextLayout::FormatRange>(), rect());
                }
                p.drawPixmap(r.left(), r.top(), blockData->buffer, 0, 0, r.width(), r.height());
            } else {
                layout->draw(&p, r.topLeft(), QVector<QTextLayout::FormatRange>(), rect());
            }
        }

        block = block.next();
    }

    overlay->buffer = map;
    overlay->update();
}

void TextmateEdit::paintEvent(QPaintEvent* e)
{
    if (rect() != overlay->buffer.rect()) {
        paintToBuffer();
    }
}

void TextmateEdit::mousePressEvent(QMouseEvent* e)
{
    if (e->modifiers() == Qt::ControlModifier) {
        addExtraCursor();
    } else {
        removeExtraCursors();
    }

    QPlainTextEdit::mousePressEvent(e);
    overlay->mousePressEvent(e);
}


static void updateCompleter(QTextDocument* doc, QCompleter* c, QString prefix)
{
    QTextCursor cursor;
    QStringList res;
    while (!(cursor = doc->find(prefix, cursor)).isNull()) {
        cursor.select(QTextCursor::WordUnderCursor);
        QString w = cursor.selectedText();
        if (w.length() <= prefix.length()) {
            continue;
        }
        if (!res.contains(w)) {
            res << w;
        }
        if (res.length() > 20) {
            break;
        }
    }
    ((QStringListModel*)c->model())->setStringList(res);
}

bool TextmateEdit::completerKeyPressEvent(QKeyEvent* e)
{
    QCompleter* c = completer;

    if (c->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return true; // let the completer do default behavior
        default:
            break;
        }
    }

    bool isShortcut = false;
    const bool ctrlOrShift = e->modifiers().testFlag(Qt::ControlModifier) || e->modifiers().testFlag(Qt::ShiftModifier);
    if (!c || (ctrlOrShift && e->text().isEmpty())) {
        return false;
    }

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
    const bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;

    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    QString completionPrefix = tc.selectedText() + e->text();

    if (!isShortcut && (hasModifier || e->text().isEmpty() || completionPrefix.length() < 2 || eow.contains(e->text().right(1)))) {
        c->popup()->hide();
        return false;
    }

    if (completionPrefix != c->completionPrefix()) {
        updateCompleter(document(), completer, completionPrefix);
        c->setCompletionPrefix(completionPrefix);
        c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
    }

    int width = c->popup()->sizeHintForColumn(0);
    if (width < 200) {
        width = 200;
    }
    QRect cr = cursorRect();
    cr.setWidth(width + c->popup()->verticalScrollBar()->sizeHint().width());
    c->complete(cr); // popup it up!
    return false;
}

void TextmateEdit::keyPressEvent(QKeyEvent* e)
{
    bool isNewline = (!(e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Enter - 1));
    bool isDelete = (!(e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace));
    if (!isDelete) {
        if (completerKeyPressEvent(e)) {
            return;
        }
    }
    if (isDelete && completer->popup()->isVisible()) {
        completer->popup()->hide();
    }

    bool handled = Commands::keyPressEvent(e);
    Editor* _editor = MainWindow::instance()->currentEditor();

    if (!handled && e->key() == Qt::Key_Tab && e->modifiers() == Qt::NoModifier) {
        if (_editor->settings->tab_to_spaces) {
            Commands::insertTab(_editor);
            handled = true;
        }
    }

    if (!handled && e->key() == Qt::Key_Escape) {
        removeExtraCursors();
    }

    if (!handled) {
        QTextCursor cursor = textCursor();
        QPlainTextEdit::keyPressEvent(e);
        updateExtraCursors(e);

        if (isNewline) {
            Commands::autoIndent(_editor);
        }
        if (!isDelete) {
            Commands::autoClose(_editor, e->text());
        }
    }

    overlay->cursorOn = true;
    overlay->update();
    paintToBuffer();
}

void TextmateEdit::wheelEvent(QWheelEvent* e)
{
    if (!editor->settings->smooth_scroll) {
        QPlainTextEdit::wheelEvent(e);
        return;
    }

    QPointF numDegrees = e->angleDelta();
    if (!numDegrees.isNull()) {
        scrollVelocity += QPointF(numDegrees.x(), numDegrees.y() * 2);
        if (!updateTimer.isActive()) {
            updateScrollDelta();
        }
    }
}

void TextmateEdit::updateScrollDelta()
{
    if (!updateTimer.isActive()) {
        updateTimer.start(50);
    }

    for (int i = 0; i < SMOOTH_SCROLL_FRAMES; i++) {

        if (scrollDelta.y() == 0 && scrollVelocity.y() == 0 && scrollDelta.x() == 0 && scrollVelocity.x() == 0) {
            updateTimer.stop();
            break;
        }

        scrollDelta += QPointF(scrollVelocity.x() * 1.0, scrollVelocity.y() * 0.6);

        float x = scrollVelocity.x() * SMOOTH_SCROLL_FRICTION_X;
        float y = scrollVelocity.y() * SMOOTH_SCROLL_FRICTION_Y;
        if (y * y < 4 && x * x < 4) {
            x = 0;
            y = 0;
        }
        scrollVelocity = QPointF(x, y);

        float scrollX = SMOOTH_SCROLL_X * devicePixelRatio(); // double for retina?

        QScrollBar* vs = verticalScrollBar();
        QScrollBar* hs = horizontalScrollBar();

        // x component
        if (scrollVelocity.x() < 0) {
            if (hs->value() >= hs->maximum()) {
                scrollDelta = QPointF(0, scrollDelta.y());
                scrollVelocity = QPointF(0, scrollVelocity.y());
                x = 0;
            }
            int scroll = 0;
            while (scrollDelta.x() < -SMOOTH_SCROLL_THRESHOLD_X) {
                scrollDelta += QPointF(SMOOTH_SCROLL_THRESHOLD_X, 0);
                scroll += scrollX;
            }
            hs->setValue(hs->value() + scroll);
        } else if (scrollVelocity.x() > 0) {
            if (hs->value() <= hs->minimum()) {
                scrollDelta = QPointF(0, scrollDelta.y());
                scrollVelocity = QPointF(0, scrollVelocity.y());
                x = 0;
            }
            int scroll = 0;
            while (scrollDelta.x() > SMOOTH_SCROLL_THRESHOLD_X) {
                scrollDelta -= QPointF(SMOOTH_SCROLL_THRESHOLD_X, scrollDelta.y());
                scroll += scrollX;
            }
            hs->setValue(hs->value() - scroll);
        }

        if (x == 0) {
            float sx = scrollDelta.x();
            if (sx * sx > 4) {
                scrollDelta += QPointF(sx * -0.2, 0);
            } else {
                scrollDelta = QPointF(0, scrollDelta.y());
            }
        }

        // y component
        if (scrollVelocity.y() < 0) {
            if (vs->value() >= vs->maximum()) {
                scrollDelta = QPointF(scrollDelta.x(), 0);
                scrollVelocity = QPointF(scrollVelocity.x(), 0);
                y = 0;
            }
            int scroll = 0;
            while (scrollDelta.y() < -SMOOTH_SCROLL_THRESHOLD_Y) {
                scrollDelta += QPointF(0, SMOOTH_SCROLL_THRESHOLD_Y);
                scroll++;
            }
            vs->setValue(vs->value() + scroll);
        } else if (scrollVelocity.y() > 0) {
            if (vs->value() <= vs->minimum()) {
                scrollDelta = QPointF(scrollDelta.x(), 0);
                scrollVelocity = QPointF(scrollVelocity.x(), 0);
                y = 0;
            }
            int scroll = 0;
            while (scrollDelta.y() > SMOOTH_SCROLL_THRESHOLD_Y) {
                scrollDelta -= QPointF(scrollDelta.x(), SMOOTH_SCROLL_THRESHOLD_Y);
                scroll++;
            }
            vs->setValue(vs->value() - scroll);
        }

        if (y == 0) {
            float sy = scrollDelta.y();
            if (sy * sy > 4) {
                scrollDelta += QPointF(0, sy * -0.2);
            } else {
                scrollDelta = QPointF(scrollDelta.x(), 0);
            }
        }
    }

    paintToBuffer();
    editor->gutter->update();
}

void TextmateEdit::insertCompletion(const QString& completion)
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    tc.insertText(completion);
    setTextCursor(tc);
}

void TextmateEdit::updateExtraCursors(QKeyEvent* e)
{
    QTextCursor cursor = textCursor();
    bool redraw = false;
    for (auto& c : extraCursors) {
        QTextCursor::MoveMode mode = e->modifiers() & Qt::ShiftModifier ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
        switch (e->key()) {
        case Qt::Key_Left:
            if (e->modifiers() & Qt::ControlModifier) {
                c.movePosition(QTextCursor::WordLeft, mode);
            } else {
                c.movePosition(QTextCursor::Left, mode);
            }
            redraw = true;
            break;
        case Qt::Key_Right:
            if (e->modifiers() & Qt::ControlModifier) {
                c.movePosition(QTextCursor::WordRight, mode);
            } else {
                c.movePosition(QTextCursor::Right, mode);
            }
            redraw = true;
            break;
        case Qt::Key_Up:
            c.movePosition(QTextCursor::Up, mode);
            redraw = true;
            break;
        case Qt::Key_Down:
            c.movePosition(QTextCursor::Down, mode);
            redraw = true;
            break;
        case Qt::Key_Backspace:
            c.deletePreviousChar();
            redraw = true;
            break;
        case Qt::Key_Delete:
            c.deleteChar();
            redraw = true;
            break;
        case Qt::Key_V:
            if (e->modifiers() & Qt::ControlModifier) {
                setTextCursor(c);
                paste();
                setTextCursor(cursor);
                redraw = true;
            }
            break;
        case Qt::Key_C:
            if (e->modifiers() & Qt::ControlModifier) {
                redraw = true;
            }
            break;
        case Qt::Key_X:
            if (e->modifiers() & Qt::ControlModifier) {
                c.removeSelectedText();
                redraw = true;
            }
            break;
        case Qt::Key_Z:
            if (e->modifiers() & Qt::ControlModifier) {
                if (e->modifiers() & Qt::ShiftModifier) {
                    document()->redo(&c);
                } else {
                    document()->undo(&c);
                }
                redraw = true;
            }
            break;
        }
    }

    if (redraw) {
        overlay->cursorOn = true;
        overlay->update();
        return;
    }

    if (!e->text().isEmpty() && (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ShiftModifier)) {
        for (auto c : extraCursors) {
            if (c.position() == cursor.position()) {
                continue;
            }
            c.insertText(e->text());
        }
    }
}

void TextmateEdit::addExtraCursor(QTextCursor cursor)
{
    if (!cursor.block().isValid()) {
        cursor = textCursor();
    }

    // check if already added
    for (auto c : extraCursors) {
        if (c.position() == cursor.position()) {
            return;
        }
        if (cursor.hasSelection() && cursor.selectionStart() == c.selectionStart() && cursor.selectedText() == c.selectedText()) {
            return;
        }
    }

    extraCursors.push_back(cursor);
}

void TextmateEdit::removeExtraCursors()
{
    extraCursors.clear();
    overlay->update();
}
