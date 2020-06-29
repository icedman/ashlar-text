
#include <QDebug>
#include <QScrollBar>
#include <QStatusBar>

#include "commands.h"
#include "mainwindow.h"

#define NO_IMPLEMENTATION(s) qDebug() << s << " not yet implemented";

static QList<QTextCursor> build_cursors(TextmateEdit* editor)
{
    QList<QTextCursor> cursors;
    cursors << editor->extraCursors;

    QTextCursor cursor = editor->textCursor();

    bool addCurrentCursor = true;
    for (auto c : cursors) {
        if (c.position() == cursor.position()) {
            addCurrentCursor = false;
            break;
        }
        if (cursor.hasSelection() && cursor.selectionStart() == c.selectionStart() && cursor.selectedText() == c.selectedText()) {
            addCurrentCursor = false;
            break;
        }
    }

    if (addCurrentCursor) {
        cursors << cursor;
    }

    return cursors;
}

size_t count_indent_size(QString s)
{
    for (int i = 0; i < s.length(); i++) {
        if (s[i] != ' ' && s[i] != " ") {
            return i;
        }
    }
    return 0;
}

QTextCursor move_to_non_whitespace(QTextCursor cursor)
{
    QTextCursor cs(cursor);
    cs.movePosition(QTextCursor::StartOfLine);
    QString s = cs.block().text();
    int i = 0;
    for (; i < s.length(); i++) {
        if (s[i] != ' ' && s[i] != '\t') {
            break;
        }
    }
    cs.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, i);
    return cs;
}

static void insertTabForCursor(Editor const* editor, QTextCursor cursor)
{
    editor_settings_ptr settings = MainWindow::instance()->editor_settings;
    if (!settings->tab_to_spaces) {
        cursor.insertText("\t");
        return;
    }

    int ts = settings->tab_size;
    QTextCursor cs(cursor);
    cs.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    size_t ws = count_indent_size(cs.selectedText() + "?");
    if (ws != 0 && ws == cursor.position() - cursor.block().position()) {
        // magic! (align to tab)
        ts = (((ws / ts) + 1) * ts) - ws;
    }

    for (int i = 0; i < ts; i++) {
        cursor.insertText(" ");
    }
}

static void Commands::insertTab(Editor const* editor)
{
    QList<QTextCursor> cursors = build_cursors(editor->editor);
    for (auto cursor : cursors) {
        insertTabForCursor(editor, cursor);
    }
}

static void Commands::removeTab(Editor const* editor, QTextCursor cursor)
{
    QTextBlock block = cursor.block();
    if (cursor.position() - block.position() == 0) {
        return;
    }

    editor_settings_ptr settings = MainWindow::instance()->editor_settings;
    if (cursor.hasSelection() || !settings->tab_to_spaces) {
        cursor.deletePreviousChar();
        return;
    }

    bool isWhiteSpace = false;
    QTextCursor cs(cursor);
    if (cs.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor)) {
        isWhiteSpace = (cs.selectedText().trimmed().length() == 0);
    }

    if (!isWhiteSpace) {
        cursor.deletePreviousChar();
        return;
    }

    for (int i = 0; i < settings->tab_size; i++) {
        cursor.deletePreviousChar();
        if ((cursor.position() - block.position()) % settings->tab_size == 0) {
            break;
        }
        QTextCursor cs(cursor);
        if (cs.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor)) {
            if (cs.selectedText().trimmed().length()) {
                break;
            }
        }
    }
}

static void toggleCommentForCursor(Editor const* editor, QTextCursor cursor)
{
    if (!editor->lang || !editor->lang->lineComment.length()) {
        return;
    }

    QString singleLineComment = editor->lang->lineComment.c_str();
    singleLineComment += " ";

    // QTextCursor cursor = editor->editor->textCursor();
    if (!cursor.hasSelection()) {
        QTextBlock block = cursor.block();
        QString s = block.text();
        int commentPosition = s.indexOf(singleLineComment);
        int hasComments = commentPosition != -1;
        size_t skip = count_indent_size(s);
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfLine);
        if (!hasComments) {
            cursor.setPosition(cursor.position() + skip);
            cursor.insertText(singleLineComment);
        } else {
            cursor.setPosition(cursor.position() + commentPosition);
            for (int i = 0; i < singleLineComment.length(); i++) {
                cursor.deleteChar();
            }
        }
        cursor.endEditBlock();
    } else {
        size_t start = cursor.selectionStart();
        QTextCursor cs(cursor);
        cs.setPosition(start);
        cs.movePosition(QTextCursor::StartOfLine);

        QTextBlock block = cs.block();
        QString s = block.text();
        int commentPosition = s.indexOf(singleLineComment);
        bool hasComments = commentPosition != -1;

        cs.beginEditBlock();
        while (cs.position() <= cursor.selectionEnd()) {
            cs.movePosition(QTextCursor::StartOfLine);
            block = cs.block();
            s = block.text().trimmed();
            if (!s.isEmpty()) {
                if (!hasComments) {
                    size_t skip = count_indent_size(s);
                    cs.setPosition(cs.position() + skip);
                    cs.insertText(singleLineComment);
                } else {
                    commentPosition = s.indexOf(singleLineComment);
                    if (commentPosition != -1) {
                        cs.setPosition(cs.position() + commentPosition);
                        for (int i = 0; i < singleLineComment.length(); i++) {
                            cs.deleteChar();
                        }
                    }
                }
            }
            if (!cs.movePosition(QTextCursor::Down)) {
                break;
            }
        }
        cs.endEditBlock();
    }
}

static void Commands::toggleComment(Editor const* editor)
{
    QList<QTextCursor> cursors = build_cursors(editor->editor);

    for (auto cursor : cursors) {
        toggleCommentForCursor(editor, cursor);
    }
}

static void toggleBlockCommentForCursor(Editor const* editor, QTextCursor cursor)
{
    if (!editor->lang || !editor->lang->blockCommentStart.length()) {
        return;
    }

    NO_IMPLEMENTATION("toggleBlockComment")
}

static void Commands::toggleBlockComment(Editor const* editor)
{
    QList<QTextCursor> cursors = build_cursors(editor->editor);

    for (auto cursor : cursors) {
        toggleBlockCommentForCursor(editor, cursor);
    }
}

static void indentForCursor(Editor const* editor, QTextCursor cursor)
{
    if (!cursor.hasSelection()) {
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfLine);
        insertTabForCursor(editor, cursor);
        cursor.endEditBlock();
    } else {
        editor_settings_ptr settings = MainWindow::instance()->editor_settings;

        size_t start = cursor.selectionStart();

        QTextCursor cs(cursor);
        cs.setPosition(start);
        cs.movePosition(QTextCursor::StartOfLine);
        if (!settings->tab_to_spaces) {
            cs.insertText("\t");
            return;
        }

        cs.beginEditBlock();
        while (cs.position() <= cursor.selectionEnd()) {
            cs.movePosition(QTextCursor::StartOfLine);
            insertTabForCursor(editor, cs);
            if (!cs.movePosition(QTextCursor::Down)) {
                break;
            }
        }
        cs.endEditBlock();
    }
}

static void Commands::indent(Editor const* editor)
{
    QList<QTextCursor> cursors = build_cursors(editor->editor);
    for (auto cursor : cursors) {
        indentForCursor(editor, cursor);
    }
}

static void unindentForCursor(Editor const* editor, QTextCursor cursor)
{
    editor_settings_ptr settings = MainWindow::instance()->editor_settings;
    if (!cursor.hasSelection()) {
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfLine);
        Commands::removeTab(editor, move_to_non_whitespace(cursor));
        cursor.endEditBlock();
    } else {
        size_t start = cursor.selectionStart();
        size_t end = cursor.selectionEnd();

        QTextCursor cs(cursor);
        cs.setPosition(start);
        cs.movePosition(QTextCursor::StartOfLine);
        cs = move_to_non_whitespace(cs);

        cs.beginEditBlock();
        while (cs.position() <= cursor.selectionEnd()) {
            Commands::removeTab(editor, move_to_non_whitespace(cs));
            if (!cs.movePosition(QTextCursor::Down)) {
                break;
            }
        }
        cs.endEditBlock();
    }
}

static void Commands::unindent(Editor const* editor)
{
    QList<QTextCursor> cursors = build_cursors(editor->editor);
    for (auto cursor : cursors) {
        unindentForCursor(editor, cursor);
    }
}

// autoIndent
static void autoIndentForCursor(Editor const* editor, QTextCursor cursor)
{
    editor_settings_ptr settings = MainWindow::instance()->editor_settings;
    int white_spaces = 0;

    HighlightBlockData* blockData;
    bool beginsWithCloseBracket = false;

    QTextCursor cs(cursor);
    QTextBlock block = cursor.block();

    if (block.isValid()) {
        blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
        if (blockData->brackets.size()) {
            beginsWithCloseBracket = !blockData->brackets[0].open;
        }
    }

    while (block.isValid()) {

        // qDebug() << block.text();

        cs.setPosition(block.position());
        cs.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
        QString blockText = block.text();
        size_t ws = count_indent_size(blockText + "?");
        if (white_spaces < ws) {
            white_spaces = ws;
        }
        if (blockText.length() && ws != 0) {
            break;
        }
        // is line whitespace
        if (ws == 0) {
            block = block.previous();
            continue;
        }

        break;
    }

    if (!block.isValid()) {
        return;
    }

    blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    if (blockData && blockData->brackets.size()) {
        auto b = blockData->brackets.back();
        if (b.open) {
            if (settings->tab_to_spaces) {
                white_spaces += settings->tab_size;
            } else {
                white_spaces++;
            }
        }
    }

    if (beginsWithCloseBracket) {
        if (settings->tab_to_spaces) {
            white_spaces -= settings->tab_size;
        } else {
            white_spaces--;
        }
    }

    // qDebug() << white_spaces;

    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfLine);
    for (int i = 0; i < white_spaces; i++) {
        if (settings->tab_to_spaces) {
            cursor.insertText(" ");
        } else {
            cursor.insertText("\t");
            i += (settings->tab_size - 1);
        }
    }
    cursor.endEditBlock();
}

static void Commands::autoIndent(Editor const* editor)
{
    QList<QTextCursor> cursors = build_cursors(editor->editor);
    for (auto cursor : cursors) {
        autoIndentForCursor(editor, cursor);
    }
}

static void autoCloseForCursor(Editor const* editor, QString lastKey, QTextCursor& cursor)
{
    if (!editor->lang || !editor->lang->pairs || lastKey.length() != 1) {
        return;
    }

    if (cursor.isNull() || cursor.hasSelection()) {
        return;
    }

    QTextCursor cs(cursor);
    cs.movePosition(QTextCursor::EndOfLine);

    // prevent duplicated bracket because of auto-close
    if (lastKey.length() == 1 && cs.position() - 1 == cursor.position()) {
        QString line = cursor.block().text();
        for (auto b : editor->lang->pairClose) {
            if (b.find(lastKey.toStdString()) == std::string::npos) {
                continue;
            }
            int pos = line.lastIndexOf(b.c_str());
            if (pos == line.length() - b.length()) {
                cursor.deleteChar();
                return;
            }
        }
    }

    // auto-close only at end of line
    if (cs.position() != cursor.position()) {
        return;
    }

    int idx = 0;
    for (auto b : editor->lang->pairOpen) {
        QString bk = b.c_str();

        int len = b.length();
        QTextCursor kc(cursor);
        if (!kc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, len)) {
            idx++;
            continue;
        }
        QString key = kc.selectedText();
        if (key != bk) {
            idx++;
            continue;
        }

        QString close = editor->lang->pairClose.at(idx).c_str();
        cursor.insertText(close);
        cursor.setPosition(cursor.position() - close.length(), QTextCursor::MoveAnchor);
        editor->editor->setTextCursor(cursor);
        return;
    }
}

static void Commands::autoClose(Editor const* editor, QString lastKey)
{
    QTextCursor mainCursor = editor->editor->textCursor();
    for (auto& cursor : editor->editor->extraCursors) {
        autoCloseForCursor(editor, lastKey, cursor);
    }

    // do main cursor last
    autoCloseForCursor(editor, lastKey, mainCursor);
}

static void duplicateLineForCursor(Editor const* editor, QTextCursor cursor)
{
    if (!cursor.hasSelection()) {
        QTextCursor cs(cursor);
        cs.select(QTextCursor::LineUnderCursor);
        QString selectedText = cs.selectedText();
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::EndOfLine);
        cursor.insertText(QString("\n") + selectedText);
        cursor.endEditBlock();
    } else {
        QString selectedText = cursor.selectedText();
        QTextCursor cs(cursor);
        cs.setPosition(cursor.selectionStart());
        cs.beginEditBlock();
        cs.insertText(selectedText);
        cs.endEditBlock();
    }
}

static void Commands::duplicateLine(Editor const* editor)
{
    QList<QTextCursor> cursors = build_cursors(editor->editor);

    for (auto cursor : cursors) {
        duplicateLineForCursor(editor, cursor);
    }
}

static QTextCursor expandSelectionToLineForCursor(Editor const* editor, QTextCursor cursor)
{
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::LineUnderCursor);
        return cursor;
        // editor->editor->setTextCursor(cursor);
    } else {
        QTextCursor cs(cursor);
        cs.setPosition(cursor.selectionStart());
        cs.movePosition(QTextCursor::StartOfLine);
        cs.setPosition(cursor.selectionEnd(), QTextCursor::KeepAnchor);
        cs.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        return cs;
        // editor->editor->setTextCursor(cs);
    }
}

static void Commands::expandSelectionToLine(Editor const* editor)
{
    QTextCursor res = expandSelectionToLineForCursor(editor, editor->editor->textCursor());
    editor->editor->setTextCursor(res);

    QList<QTextCursor> cursors;
    for (auto cursor : editor->editor->extraCursors) {
        cursors << expandSelectionToLineForCursor(editor, cursor);
    }

    editor->editor->extraCursors.clear();
    editor->editor->extraCursors << cursors;
}

static bool Commands::find(Editor const* editor, QString string, QString options)
{
    if (string.isEmpty()) {
        return false;
    }

    TextmateEdit* e = editor->editor;
    int scroll = e->verticalScrollBar()->value();
    bool regex = options.indexOf("regular_") != -1;
    int flags = 0;
    if (options.indexOf("case_") != -1) {
        flags = QTextDocument::FindCaseSensitively;
    }
    if (options.indexOf("whole_") != -1) {
        flags |= QTextDocument::FindWholeWords;
    }

    if (!regex) {
        if (!e->find(string, flags)) {
            QTextCursor cursor = e->textCursor();
            QTextCursor cs(cursor);
            cs.movePosition(QTextCursor::Start);
            e->setTextCursor(cs);
            if (!e->find(string, flags)) {
                e->setTextCursor(cursor);
                e->verticalScrollBar()->setValue(scroll);
                MainWindow::instance()->statusBar()->showMessage("Unable to find string", 2000);
                return false;
            }
        }

        // e->centerCursor();
        return true;
    }

    QRegExp regx(string);
    if (!e->find(regx, flags)) {
        QTextCursor cursor = e->textCursor();
        QTextCursor cs(cursor);
        cs.movePosition(QTextCursor::Start);
        e->setTextCursor(cs);
        if (!e->find(regx, flags)) {
            e->setTextCursor(cursor);
            e->verticalScrollBar()->setValue(scroll);
            MainWindow::instance()->statusBar()->showMessage("Unable to find string", 2000);
            return false;
        }
    }

    // e->centerCursor();
    return true;
}

static bool Commands::keyPressEvent(QKeyEvent* e)
{
    QString keys = QKeySequence(e->modifiers() | e->key()).toString().toLower();
    QVariant value;
    if (e->modifiers() != Qt::NoModifier) {
        value = MainWindow::instance()->js()->runScript("try { keybinding.processKeys(\"" + keys + "\"); } catch(err) { console.log(err) } ");
    }

    MainWindow::instance()->js()->runScript("try { ashlar.events.emit(\"keyPressed\", \"" + keys + "\"); } catch(err) { console.log(err) } ");
    return value.toBool();
}