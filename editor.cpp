#include <QtWidgets>
#include <iostream>

#include "commands.h"
#include "editor.h"
#include "gutter.h"
#include "mainwindow.h"
#include "minimap.h"
#include "reader.h"
#include "settings.h"

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
    connect(&watcher, SIGNAL(fileChanged(const QString &)), this, SLOT(fileChanged(const QString &)));
}

Editor::~Editor()
{
}

void Editor::newFile()
{
    editor->clear();
    fileName = "";
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

void Editor::fileChanged(const QString &path)
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
        QPalette p = editor->palette();
        if (isDark) {
            p.setColor(QPalette::Highlight, selectionBgColor.lighter(200));
        } else {
            p.setColor(QPalette::Highlight, selectionBgColor.darker(200));
        }
        editor->setPalette(p);
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

    gutter = new Gutter();
    gutter->font = font;
    gutter->editor = this;

    mini = new MiniMap();
    mini->editor = this;

    vscroll = new QScrollBar();

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
        if (blockData->brackets.size()) {
            auto l = blockData->brackets.back();
            return l.open;
        }
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
    int index = 0;
    while (block.isValid()) {
        if (block.isVisible()) {
            QRectF rect = editor->_blockBoundingGeometry(block).translated(editor->_contentOffset());
            if (sidebarRect.intersects(rect)) {
                if (gutter->lineNumbers.count() >= index)
                    gutter->lineNumbers.resize(index + 1);
                gutter->lineNumbers[index].position = rect.top();
                gutter->lineNumbers[index].number = block.blockNumber() + 1;
                gutter->lineNumbers[index].foldable = isFoldable(block);
                ++index;
            }
            if (rect.top() > sidebarRect.bottom())
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
}

static QTextBlock findBracketMatch(QTextBlock& block)
{
    if (!block.isValid()) {
        return QTextBlock();
    }

    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    std::vector<bracket_info_t> brackets = blockData->brackets;

    if (!brackets.size()) {
        return QTextBlock();
    }

    QTextBlock res = block.next();
    while (res.isValid()) {
        HighlightBlockData* resData = reinterpret_cast<HighlightBlockData*>(res.userData());
        if (!resData) {
            continue;
        }
        std::vector<bracket_info_t> resBrackets = resData->brackets;
        for (auto b : resBrackets) {
            if (!b.open) {
                auto l = brackets.back();
                if (l.open && l.bracket == b.bracket) {
                    brackets.pop_back();
                } else {
                    return QTextBlock();
                }

                if (!brackets.size()) {
                    // std::cout << "found end!" << std::endl;
                    return res;
                }
                continue;
            }
            brackets.push_back(b);
        }
        res = res.next();
    }

    return res;
}

void Editor::toggleFold(size_t line)
{
    QTextDocument* doc = editor->document();
    QTextBlock folder = doc->findBlockByNumber(line - 1);
    QTextBlock end = findBracketMatch(folder);
    QTextBlock block = doc->findBlockByNumber(line);

    if (!end.isValid() || !folder.isValid() || !block.isValid()) {
        return;
    }

    // todo.. can't handle ))
    HighlightBlockData* folderBlockData = reinterpret_cast<HighlightBlockData*>(folder.userData());
    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());

    if (folderBlockData && blockData) {
        folderBlockData->folded = !folderBlockData->folded;
        while (block.isValid()) {
            blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
            blockData->folded = folderBlockData->folded;
            if (folderBlockData->folded) {
                block.setVisible(false);
                block.setLineCount(0);
            } else {
                block.setVisible(true);
                block.setLineCount(1);
            }

            if (block == end) {
                break;
            }
            block = block.next();
        }
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
    // std::cout << "still got ot me" << std::endl;
    // listening to click events
}

static void updateCompleter(QTextDocument* doc, QCompleter* c, QString prefix, QTextBlock currentBlock)
{
    QTextCursor cursor;
    QStringList res;
    // qDebug() << prefix;
    while (!(cursor = doc->find(prefix, cursor)).isNull()) {
        if (cursor.block() == currentBlock) {
            continue;
        }
        cursor.select(QTextCursor::WordUnderCursor);
        QString w = cursor.selectedText();
        // if (w != prefix && !res.contains(w)) {
        if (!res.contains(w)) {
            res << w;
        }
        if (res.length() > 20) {
            break;
        }
    }
    ((QStringListModel*)c->model())->setStringList(res);
}

//---------------------
// custom QPlainTextEdit
//---------------------
TextmateEdit::TextmateEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    overlay = new Overlay(this);
    editor = (Editor*)parent;

    completer = new QCompleter(this);
    completer->setModel(new QStringListModel());
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(false);
    completer->setWidget(this);

    QObject::connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
        this, &TextmateEdit::insertCompletion);
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

    //-----------------
    // selections
    //-----------------
    for (auto cursor : cursors) {
        if (!cursor.hasSelection()) {
            continue;
        }

        QTextCursor cs(cursor);
        cs.setPosition(cs.selectionStart());
        // qDebug() << "[" << cursor.selectionStart() << "," << cursor.selectionEnd() << "]";

        while (cs.position() < cursor.selectionEnd()) {
            QTextBlock block = cs.block();
            QRectF r = editor->_blockBoundingGeometry(block).translated(editor->_contentOffset());
            if (r.top() > height()) {
                break;
            }

            if (block.isVisible()) {
                QTextLayout* layout = block.layout();

                for (int i = 0; i < layout->lineCount(); i++) {
                    QTextLine line = layout->lineAt(i);
                    int sx = line.textStart();

                    // qDebug() << "#" << i << " sx:" << sx;
                    if (sx + block.position() < cursor.selectionStart()) {
                        sx = cursor.selectionStart() - block.position();
                        // qDebug() << "adjust:" << sx;
                    }

                    int ex = sx + line.textLength();

                    // qDebug() << "#" << i << " ex:" << ex;
                    if (ex + block.position() > cursor.selectionEnd()) {
                        ex = cursor.selectionEnd() - block.position();
                        // qDebug() << "adjust:" << ex;
                    }

                    qreal srx = line.cursorToX(&sx);
                    qreal erx = line.cursorToX(&ex);
                    float w = erx - srx;
                    float h = fh;

                    // qDebug() << "(" << sx << "," << ex << ")" << " [" << srx << "-" << erx << "]";

                    r.setWidth(w);
                    p.fillRect(QRect(r.left() + srx, r.top() + (i * fh), w, h), e->selectionBgColor);
                }
            }

            if (!cs.movePosition(QTextCursor::Down)) {
                break;
            }
            cs.movePosition(QTextCursor::StartOfLine);
        }
    }

    block = firstVisibleBlock();
    while (block.isValid()) {
        QRectF r = blockBoundingGeometry(block).translated(contentOffset());
        if (r.top() > height()) {
            break;
        }

        if (block.isVisible()) {
            QTextLayout* layout = block.layout();

            //-----------------
            // folded indicator
            //-----------------
            HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
            if (blockData && blockData->folded) {
                p.fillRect(r, foldedBg);
            }

            //-----------------
            // render the block
            //-----------------
            // layout->draw(&p, r.topLeft());
            QVector<QTextLayout::FormatRange> selectionFormats;
            // if (blockFormats.contains(block.firstLineNumber())) {
            //     selectionFormats = blockFormats[block.firstLineNumber()];
            // }
            layout->draw(&p, r.topLeft(), selectionFormats, rect());
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
        updateCompleter(document(), completer, completionPrefix, tc.block());
        c->setCompletionPrefix(completionPrefix);
        c->popup()->setCurrentIndex(c->completionModel()->index(0,0));
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
                // todo pastes only from the main cursor
                setTextCursor(c);
                paste();
                setTextCursor(cursor);
                redraw = true;
            }
            break;
        case Qt::Key_C:
            if (e->modifiers() & Qt::ControlModifier) {
                // todo copies only from the main cursor
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
