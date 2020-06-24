#ifndef JS_H
#define JS_H

#include <QObject>

class JSFs : public QObject {
    Q_OBJECT

public:
public slots:
    QString readFile(QString path);
    bool appendFile(QString content, QString path);
    bool writeFile(QString content, QString path);
};

class Editor;
class JSApp : public QObject {
    Q_OBJECT

public:
    JSApp(QObject* parent = 0);

public slots:

    bool beginEditor(QString id);
    void endEditor();
    QStringList editors();
    QString currentEditor();

    void log(QString log);

    void tab(int i);
    void newTab();
    void closeTab();
    void exit();
    void theme(QString name);

    QString projectPath();

    // active editor
    void clear();
    void appendHtml(QString html);
    void appendText(QString text);
    void toggleComment();
    void toggleBlockComment();
    void indent();
    void unindent();
    void duplicateLine();
    void expandSelectionToLine();
    void zoomIn();
    void zoomOut();
    void addExtraCursor();
    void removeExtraCursors();
    bool find(QString string, QString options = QString());
    bool findAndCreateCursor(QString string, QString options = QString());
    QString selectedText();
    QList<int> cursor();

    // debug
    QStringList scopesAtCursor();
    QString language();

    void reloadExtensions();
    // void loadExtension(QString name);
    void runScriptFile(QString path);

    void showInspector(bool showHtml);

private:
    Editor* editor();
    Editor* _editor;
};

#endif // JS_H