#ifndef JS_H
#define JS_H

#include <QObject>

class Process;
class JSFs : public QObject {
    Q_OBJECT
public slots:
    QString readFile(QString path);
    QString readFileSync(QString path, QString options);
    bool appendFile(QString content, QString path);
    bool writeFile(QString content, QString path);
};

class JSPs : public QObject {
    Q_OBJECT
public slots:

    int git();
    int fzf();

    int run(int idx, QStringList args);
    void write(int idx, QString input);

private Q_SLOTS:
    void output(Process* process, QString out);
    void finished(Process* process, int code);

private:
    Process* findProcess(int id);
    int requestProcess();
    QList<Process*> processes;
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

    void toggleSidebar();

    QString settings();
    void updateSettings(QString settings);

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
    void setCursor(int line, int position, bool select);
    void centerCursor();
    bool find(QString string, QString options = QString());
    bool findAndCreateCursor(QString string, QString options = QString());
    QString selectedText();
    QList<int> cursor();

    QStringList allFiles();
    void openFile(QString path);

    // debug
    QStringList scopesAtCursor();
    QString language();

    void loadExtensions();
    // void loadExtension(QString name);
    void runScriptFile(QString path);

    void showCommandPalette();
    void showInspector(bool showHtml);
    void hideInspector();

private:
    Editor* editor();
    Editor* _editor;
};

#endif // JS_H