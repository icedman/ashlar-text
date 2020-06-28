#include <QtWidgets>

#include "tmedit.h"
#include "editor.h"
#include "gutter.h"
#include "minimap.h"
#include "commands.h"
#include "mainwindow.h"

#define SMOOTH_SCROLL_THRESHOLD_X 400
#define SMOOTH_SCROLL_THRESHOLD_Y 400
#define SMOOTH_SCROLL_FRAMES 2
#define SMOOTH_SCROLL_FRICTION_X 0.9
#define SMOOTH_SCROLL_FRICTION_Y 0.8
#define SMOOTH_SCROLL_X 8

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
        
        QTextCursor end = e->findBracketMatchCursor(bracket, editor->textCursor());
        end.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        
        if (!start.isNull() && !end.isNull()) {
            pairs.push_back(start);
            pairs.push_back(end);
            cursors << start;
            cursors << end;
        } else {
            if (!start.isNull()) {
                qDebug() << "unpaired";
            }
        }
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
                    QColor selectionColor = e->selectionBgColor;

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
                        h = 2;
                    }

                    r.setWidth(w);
                    p.fillRect(QRect(r.left() + srx, r.top() + (i * fh) + offsetY, w, h), selectionColor);
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
    bool handledForMainCursor = false;
    bool noModifierExceptShift = (e->modifiers() == Qt::NoModifier || e->modifiers() & Qt::ShiftModifier);
    bool isNewline = (!(e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Enter - 1));
    bool isDelete = (!(e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace));
    bool isPaste = ((e->modifiers() & Qt::ControlModifier && e->key() == Qt::Key_V) ||
        (e->modifiers() & Qt::ShiftModifier && e->key() == Qt::Key_Insert)) && canPaste();

    // completer        
    if (!isDelete) {
        if (completerKeyPressEvent(e)) {
            return;
        }
    }
    if (isDelete && completer->popup()->isVisible()) {
        completer->popup()->hide();
    }

    // keybinding
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

    if ((!(e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Backspace)) {
        QTextCursor cursor = textCursor();
        if (cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor)) {
            QString st = cursor.selectedText();
            if (st[0] == ' ' || st[0] == '\t') {
                Commands::removeTab(_editor, textCursor());
                handledForMainCursor = true;
            }
        }
    }
    
    if (isNewline) {
        QTextCursor cursor = textCursor();
        size_t ws = count_indent_size(cursor.block().text() + "?");
        if (ws >= cursor.position() - cursor.block().position()) {
            cursor.beginEditBlock();
            cursor.movePosition(QTextCursor::Up);
            cursor.movePosition(QTextCursor::EndOfLine);
            cursor.insertText("\n");
            cursor.endEditBlock();
            updateExtraCursors(e);
            handled = true;
        }
    }
        
    if (!handled) {
        QTextCursor cursor = textCursor();
        
        // todo.. defer only with large clipboard texts
        if (isPaste) {
            editor->highlighter->setDeferRendering(true);
            // qDebug() << "check clipboard text size here";    
        }

        if (!handledForMainCursor) {
            QPlainTextEdit::keyPressEvent(e);
        }
        updateExtraCursors(e);
        
         if (isPaste) {
             editor->highlightBlocks();
         }
                
        // autos
        if (isNewline) {
            Commands::autoIndent(_editor);
        }
        if (noModifierExceptShift && !isDelete) {
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
    bool isNewline = (!(e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Enter - 1));
    
    Editor *editor = (Editor*)parent();
    QTextCursor cursor = textCursor();
    bool redraw = false;
    for (auto& c : extraCursors) {
        
        if (isNewline) {
            QTextCursor cursor = c;
            size_t ws = count_indent_size(cursor.block().text() + "?");
            if (ws >= cursor.position() - cursor.block().position()) {
                cursor.beginEditBlock();
                cursor.movePosition(QTextCursor::Up);
                cursor.movePosition(QTextCursor::EndOfLine);
                cursor.insertText("\n");
                cursor.endEditBlock();
                redraw = true;
                continue;
            }
        }
        
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
        case Qt::Key_Delete:
            c.deleteChar();
            redraw = true;
            break;
        case Qt::Key_Backspace:
            {
            bool handled = false;
            
            QTextCursor prev(c);
            if (prev.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor)) {
                QString st = prev.selectedText();
                if (st[0] == ' ' || st[0] == '\t') {
                    Commands::removeTab(editor, c);
                    handled = true;
                }
            }
            if (!handled) {
                c.deletePreviousChar();
            }
        
            redraw = true;
            }
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
