#ifndef JS_H
#define JS_H

#include <QObject>

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
       
    void showInspector(bool showHtml);

private:

    Editor* editor();
    Editor* _editor;
};

#endif // JS_H