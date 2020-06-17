#ifndef COMMANDS_H
#define COMMANDS_H

#include <QTextCursor>
#include <QKeyEvent>

#include "editor.h"

class Commands {
public:
    static void removeTab(Editor const* editor, QTextCursor cursor);
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
