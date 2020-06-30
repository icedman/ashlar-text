#ifndef COMMANDS_H
#define COMMANDS_H

#include <QKeyEvent>
#include <QTextCursor>

#include "editor.h"
#include "tmedit.h"

size_t count_indent_size(QString s);
QTextCursor move_to_non_whitespace(QTextCursor cursor);

class Commands {
public:
    static bool removeTab(Editor const* editor, QTextCursor cursor);
    static void insertTab(Editor const* editor);
    static void toggleComment(Editor const* editor);
    static void toggleBlockComment(Editor const* editor);
    static void autoIndent(Editor const* editor);
    static void autoClose(Editor const* editor, QString lastKey);
    static void indent(Editor const* editor);
    static void unindent(Editor const* editor);
    static void duplicateLine(Editor const* editor);
    static void expandSelectionToLine(Editor const* editor);
    static bool find(Editor const* editor, QString words, QString options);

    static bool keyPressEvent(QKeyEvent* e);
};

#endif // COMMANDS_H
