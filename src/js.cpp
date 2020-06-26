#include <QBoxLayout>
#include <QDebug>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "commands.h"
#include "editor.h"
#include "mainwindow.h"

#include "js.h"

static QString sanitizePath(QString path)
{
    QString projectPath = MainWindow::instance()->projectPath;
    path = path.replace("~", "");
    path = path.replace("..", "");
    if ((path + "/").contains(QFileInfo(projectPath).absolutePath())) {
        projectPath = "";
    }
    qDebug() << path;
    qDebug() << projectPath;
    return QFileInfo(projectPath + "/" + path).absoluteFilePath();
}

//-----------------
// limited fs
//-----------------
QString JSFs::readFile(QString path)
{
    QFile file(sanitizePath(path));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        return file.readAll();
    }
    return "";
}

QString JSFs::readFileSync(QString path, QString options)
{
    return readFile(path);
}

bool JSFs::appendFile(QString content, QString path)
{
    QFile file(sanitizePath(path));
    if (file.open(QFile::Append | QFile::Text)) {
        QTextStream out(&file);
        out << content;
    }
    return true;
}

bool JSFs::writeFile(QString content, QString path)
{
    QFile file(sanitizePath(path));
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        out << content;
    }
    return true;
}

//-----------------
// main app
//-----------------
JSApp::JSApp(QObject* parent)
    : QObject(parent)
    , _editor(0)
{
}

void JSApp::log(QString log)
{
    qDebug() << log;
}

void JSApp::tab(int i)
{
    MainWindow* mw = MainWindow::instance();
    mw->tabSelected(i);
}

void JSApp::newTab()
{
    MainWindow* mw = MainWindow::instance();
    mw->newFile();
}

void JSApp::closeTab()
{
    MainWindow* mw = MainWindow::instance();
    mw->tabClose(mw->currentTab());
}

void JSApp::exit()
{
    MainWindow::instance()->close();
}

bool JSApp::beginEditor(QString id)
{
    MainWindow* mw = MainWindow::instance();
    _editor = mw->findEditor(id);
    return _editor != 0;
}

void JSApp::endEditor()
{
    _editor = 0;
}

QStringList JSApp::editors()
{
    MainWindow* mw = MainWindow::instance();
    return mw->editorsPath();
}

Editor* JSApp::editor()
{
    if (_editor) {
        return _editor;
    }
    MainWindow* mw = MainWindow::instance();
    return mw->currentEditor();
}

QString JSApp::currentEditor()
{
    return editor()->fullPath();
}

void JSApp::theme(QString name)
{
    MainWindow* mw = MainWindow::instance();
    mw->loadTheme(name);
}

QString JSApp::projectPath()
{
    MainWindow* mw = MainWindow::instance();
    return mw->projectPath;
}

void JSApp::clear()
{
    editor()->editor->clear();
}

void JSApp::appendHtml(QString html)
{
    editor()->editor->appendHtml(html);
}

void JSApp::appendText(QString text)
{
    editor()->editor->appendPlainText(text);
}

void JSApp::toggleComment()
{
    Commands::toggleComment(editor());
}

void JSApp::toggleBlockComment()
{
    Commands::toggleBlockComment(editor());
}

void JSApp::indent()
{
    Commands::indent(editor());
}

void JSApp::unindent()
{
    Commands::unindent(editor());
}

void JSApp::duplicateLine()
{
    Commands::duplicateLine(editor());
}

void JSApp::expandSelectionToLine()
{
    Commands::expandSelectionToLine(editor());
}

QString JSApp::selectedText()
{
    return editor()->editor->textCursor().selectedText();
}

QList<int> JSApp::cursor()
{
    QList<int> res;
    QTextCursor cs = editor()->editor->textCursor();
    QTextBlock block = cs.block();
    if (!block.isValid()) {
        return res;
    }
    res << block.firstLineNumber();
    res << cs.position() - block.position();
    return res;
}

void JSApp::zoomIn()
{
    editor()->editor->zoomIn();
    // editor()->invalidateBuffers();
}

void JSApp::zoomOut()
{
    editor()->editor->zoomOut();
    // editor()->invalidateBuffers();
}

void JSApp::addExtraCursor()
{
    editor()->editor->addExtraCursor();
}

void JSApp::removeExtraCursors()
{
    editor()->editor->removeExtraCursors();
}

bool JSApp::find(QString string, QString options)
{
    return Commands::find(editor(), string, options);
}

bool JSApp::findAndCreateCursor(QString string, QString options)
{
    QTextCursor prev = editor()->editor->textCursor();
    if (string.isEmpty()) {
        prev.select(QTextCursor::WordUnderCursor);
        editor()->editor->setTextCursor(prev);
        return !prev.selectedText().isEmpty();
    }

    bool res = Commands::find(editor(), string, options);
    if (res) {
        editor()->editor->addExtraCursor(prev);
    }
    return res;
}

void JSApp::showCommandPalette()
{
    MainWindow::instance()->showCommandPalette();
}

void JSApp::showInspector(bool showHtml)
{
    MainWindow* mw = MainWindow::instance();
    mw->js()->showInspector(showHtml);
}

void JSApp::hideInspector()
{
    MainWindow* mw = MainWindow::instance();
    mw->js()->hideInspector();
}

QStringList JSApp::scopesAtCursor()
{
    return editor()->scopesAtCursor(editor()->editor->textCursor());
}

QString JSApp::language()
{
    if (!editor()->lang) {
        return "";
    }
    return editor()->lang->id.c_str();
}

void JSApp::runScriptFile(QString path)
{
    MainWindow::instance()->js()->runScriptFile(path);
}

void JSApp::reloadExtensions()
{
    MainWindow::instance()->loadAllExtensions();
}

QStringList JSApp::allFiles()
{
    return MainWindow::instance()->allFiles();
}

void JSApp::openFile(QString path)
{
    QString sanitized = sanitizePath(path);
    qDebug() << sanitized;
    MainWindow::instance()->openFile(sanitized);
}