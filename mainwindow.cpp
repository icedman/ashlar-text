#include <QStackedWidget>
#include <QStandardPaths>
#include <QtWidgets>

#include <iostream>

#include "commands.h"
#include "mainwindow.h"
#include "reader.h"
#include "settings.h"
#include "theme.h"

#include "qt/core.h"

static MainWindow* _instance;

#define UNTITLED_TEXT tr("untitled")

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , updateTimer(this)
    , engine(new Engine)
    , jsApp(this)
    , icons(0)
{
    _instance = this;
    configure();

    // setWindowFlags(Qt::FramelessWindowHint);

    setupLayout();
    setupMenu();

    applySettings();
    applyTheme();

    setWindowTitle(tr("Editor"));
    setMinimumSize(600, 400);

    updateTimer.singleShot(250, this, SLOT(warmConfigure()));
}

MainWindow::~MainWindow()
{
    delete engine;
}

MainWindow* MainWindow::instance()
{
    return _instance;
}

Editor* MainWindow::currentEditor() { return (Editor*)editors->currentWidget(); }

QStringList MainWindow::editorsPath()
{
    QStringList res;
    for (int i = 0; i < editors->count(); i++) {
        Editor* e = qobject_cast<Editor*>(editors->widget(i));
        if (e) {
            res << e->fullPath();
        }
    }
    return res;
}

Editor* MainWindow::findEditor(QString path)
{
    for (int i = 0; i < editors->count(); i++) {
        Editor* e = qobject_cast<Editor*>(editors->widget(i));
        if (e && e->fullPath() == path) {
            return e;
        }
    }
    return 0;
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Syntax Highlighter"),
        tr("<p>The <b>Syntax Highlighter</b> example shows how "
           "to perform simple syntax highlighting by subclassing "
           "the QSyntaxHighlighter class and describing "
           "highlighting rules using regular expressions.</p>"));
}

void MainWindow::configure()
{
    editor_settings = std::make_shared<editor_settings_t>();
    QString userSettings = QStandardPaths::locate(QStandardPaths::HomeLocation, ".ashlar", QStandardPaths::LocateDirectory);
    load_settings(userSettings, settings);

    QString userExtensions = QStandardPaths::locate(QStandardPaths::HomeLocation, ".ashlar/extensions", QStandardPaths::LocateDirectory);
    load_extensions(userExtensions, extensions);

    if (settings.isMember("extensions")) {
        Json::Value exts = settings["extensions"];
        if (exts.isArray()) {
            for (auto path : exts) {
                load_extensions(QString(path.asString().c_str()), extensions);
            }
        }
    }

    // load_extensions(QString("./extensions"), extensions);

    if (settings["theme"].isString()) {
        theme = theme_from_name(settings["theme"].asString().c_str(), extensions);
    } else {
        theme = theme_from_name("Monokai", extensions);
    }

    if (settings["icon_theme"].isString()) {
        icons = icon_theme_from_name(settings["icon_theme"].asString().c_str(), extensions);
    }

    // editor settings
    editor_settings->mini_map = settings.isMember("mini_map") && settings["mini_map"] == true;
    editor_settings->gutter = settings.isMember("gutter") && settings["gutter"] == true;

    if (settings.isMember("font")) {
        editor_settings->font = settings["font"].asString();
    } else {
        editor_settings->font = "monospace";
    }

    if (settings.isMember("font_size")) {
        editor_settings->font_size = std::stof(settings["font_size"].asString());
    } else {
        editor_settings->font_size = 12;
    }

    if (settings.isMember("tab_size")) {
        editor_settings->tab_size = std::stoi(settings["tab_size"].asString());
    } else {
        editor_settings->tab_size = 4;
    }

    editor_settings->tab_to_spaces = settings.isMember("tab_to_spaces") && settings["tab_to_spaces"] == true;
    editor_settings->word_wrap = settings.isMember("word_wrap") && settings["word_wrap"] == true;
    editor_settings->auto_indent = settings.isMember("auto_indent") && settings["auto_indent"] == true;
    editor_settings->auto_close = settings.isMember("auto_close") && settings["auto_close"] == true;

    // std::cout << settings << std::endl;

    // fix invalids
    if (editor_settings->font_size < 6) {
        editor_settings->font_size = 6;
    }
    if (editor_settings->font_size > 48) {
        editor_settings->font_size = 48;
    }
    if (editor_settings->tab_size < 1) {
        editor_settings->tab_size = 1;
    }
    if (editor_settings->tab_size > 8) {
        editor_settings->tab_size = 8;
    }
}

void MainWindow::loadTheme(const QString& name)
{
    theme_ptr _theme = theme_from_name(name, extensions);
    if (_theme) {
        // swap(theme, _theme);
        theme = _theme;
        applyTheme();
    }
}

void MainWindow::applyTheme()
{
    theme_application(theme);

    // update all editors
    for (int i = 0; i < editors->count(); i++) {
        Editor* editor = (Editor*)editors->widget(i);
        editor->setTheme(theme);
    }
}

void MainWindow::setHost(QString path)
{
    hostPath = path;
}

void MainWindow::applySettings()
{
    if (settings.isMember("sidebar") && settings["sidebar"] == true) {
        QFont font;
        font.setFamily(editor_settings->font.c_str());
        font.setPointSize(editor_settings->font_size);
        font.setFixedPitch(true);
        sidebar->setFont(font);
        sidebar->show();
    } else {
        sidebar->hide();
    }

    if (settings.isMember("statusbar") == true) {
        QFont font;
        font.setFamily(editor_settings->font.c_str());
        font.setPointSize(editor_settings->font_size);
        font.setFixedPitch(true);
        statusBar()->setFont(font);

        statusBar()->show();
    } else {
        statusBar()->hide();
    }
}

void MainWindow::setupLayout()
{
    splitterv = new QSplitter(Qt::Vertical);
    splitter = new QSplitter(Qt::Horizontal);
    editors = new QStackedWidget();

    sidebar = new Sidebar(this);
    sidebar->mainWindow = this;

    tabs = new Tabs(this);
    tabs->setDrawBase(false);
    tabs->setExpanding(false);
    tabs->setTabsClosable(false);

    connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(tabSelected(int)));
    connect(tabs, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClose(int)));

    QWidget* mainPane = new QWidget();
    QVBoxLayout* vbox = new QVBoxLayout();

    mainPane->setLayout(vbox);
    vbox->addWidget(tabs);
    vbox->addWidget(editors);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    splitterv->addWidget(splitter);
    // splitterv->setStretchFactor(3, 1);

    splitter->addWidget(sidebar);
    splitter->addWidget(mainPane);
    splitter->setStretchFactor(1, 3);

    setCentralWidget(splitterv);
}

void MainWindow::newFile()
{
    int tabIdx = tabs->addTab(UNTITLED_TEXT);
    Editor* editor = createEditor();
    editors->addWidget(editor);
    tabs->setTabData(tabIdx, QVariant::fromValue(editor));
    tabSelected(tabIdx);
}

void MainWindow::saveFile(bool saveNew)
{
    QString fileName = currentEditor()->fileName;

    if (QFileInfo(fileName).fileName() == UNTITLED_TEXT) {
        fileName = "";
    }

    // std::cout << fileName.toUtf8().constData() << std::endl;
    if (saveNew || fileName.isNull() || fileName.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", "");
    }

    if (!fileName.isEmpty()) {
        if (currentEditor()->saveFile(QFileInfo(fileName).absoluteFilePath())) {
            tabs->setTabText(tabs->currentIndex(), QFileInfo(currentEditor()->fileName).fileName());
            statusBar()->showMessage("Saved " + currentEditor()->fileName, 5000);
        }
    }
}

void MainWindow::saveFileAs()
{
    saveFile(true);
}

void MainWindow::openFile(const QString& path)
{
    QString fileName = path;

    if (fileName.isNull())
        fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", "C, C++ Files (*.c *.cpp *.h)");

    if (fileName.isEmpty()) {
        return;
    }

    // directory open requested
    if (QFileInfo(fileName).isDir()) {
        if (tabs->count() == 0) {
            if (!editors->count()) {
                projectPath = fileName;
                sidebar->setRootPath(projectPath);
            }
            newFile();
        }
        return;
    }

    if (QFile::exists(fileName)) {
        fileName = QFileInfo(fileName).absoluteFilePath();

        if (tabs->count() == 0) {
            projectPath = QFileInfo(fileName).path();
            sidebar->setRootPath(projectPath);
            sidebar->hide();
        }

        openTab(fileName);
        if (currentEditor()->fileName != fileName) {
            currentEditor()->setLanguage(language_from_file(fileName, extensions));
            currentEditor()->openFile(fileName);
        }

        return;
    } else {
        if (!tabs->count()) {
            newFile();
        }
    }
}

Editor* MainWindow::createEditor()
{
    Editor* editor = new Editor(this);
    editor->settings = editor_settings;
    editor->setTheme(theme);
    editor->setupEditor();
    return editor;
}

void MainWindow::tabSelected(int index)
{
    if (index >= tabs->count()) {
        index = tabs->count() - 1;
    }

    // std::cout << "Tabs:" << tabs->count() << std::endl;
    // std::cout << "Editors:" << editors->count() << std::endl;
    // std::cout << "Selected:" << index << std::endl;
    if (index != -1) {
        QVariant data = tabs->tabData(index);
        Editor* _editor = qvariant_cast<Editor*>(data);
        if (_editor) {
            editors->setCurrentWidget(_editor);
            tabs->setCurrentIndex(index);
            _editor->editor->setFocus(Qt::ActiveWindowFocusReason);
        }
    }
}

void MainWindow::tabClose(int index)
{
    if (tabs->count() == 1) {
        if (tabs->tabText(0) == UNTITLED_TEXT) {
            close();
            return;
        }
    }

    // std::cout << "Close " << index << std::endl;
    if (index >= 0 && index < tabs->count()) {
        tabSelected(index);
        QVariant data = tabs->tabData(index);
        Editor* _editor = qvariant_cast<Editor*>(data);
        if (_editor) {
            editors->removeWidget(_editor);
            tabs->removeTab(index);
            _editor->deleteLater();
        }
    }

    if (!tabs->count()) {
        newFile();
        return;
    }

    tabSelected(index);
}

Editor* MainWindow::openTab(const QString& _path)
{
    QString path = _path;

    int tabIdx = -1;
    for (int i = 0; i < tabs->count(); i++) {
        QVariant data = tabs->tabData(i);
        Editor* _editor = qvariant_cast<Editor*>(data);
        if (_editor->fileName == path) {
            tabIdx = i;
            std::cout << i << std::endl;
            break;
        }
    }

    if (tabIdx != -1) {
        tabSelected(tabIdx);
        return currentEditor();
    }

    QString _fileName = QFileInfo(path).fileName();
    if (_fileName.isEmpty()) {
        _fileName = UNTITLED_TEXT;
        path = QFileInfo(_fileName).absoluteFilePath();
    }

    tabIdx = tabs->addTab(_fileName);
    Editor* _editor = createEditor(); // << creates a new editor
    editors->addWidget(_editor);
    tabs->setTabData(tabIdx, QVariant::fromValue(_editor));
    tabSelected(tabIdx);
    return _editor;
}

void MainWindow::setupMenu()
{
    // File
    fileMenu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(fileMenu);

    fileMenu->addAction(tr("&New"), this,
        &MainWindow::newFile, QKeySequence::New);
    fileMenu->addAction(
        tr("&Open..."),
        this, [this]() { openFile(); }, QKeySequence::Open);
    fileMenu->addAction(
        tr("&Save..."),
        this, [this]() { saveFile(); }, QKeySequence::Save);
    fileMenu->addAction(
        tr("Save As..."),
        this, [this]() { saveFileAs(); }, QKeySequence::SaveAs);
    fileMenu->addAction(tr("E&xit"), qApp,
        &QApplication::quit, QKeySequence::Quit);

    // View
    viewMenu = new QMenu(tr("&View"), this);
    viewMenu->addAction(
        tr("Toggle Sidebar"),
        this, [this]() { sidebar->setVisible(!sidebar->isVisible()); });
    viewMenu->addAction(
        tr("Toggle Minimap"),
        this, [this]() { editor_settings->mini_map = !editor_settings->mini_map; currentEditor()->hide(); currentEditor()->show(); });
    viewMenu->addAction(
        tr("Toggle Statusbar"),
        this, [this]() { statusBar()->setVisible(!statusBar()->isVisible()); });
    menuBar()->addMenu(viewMenu);

    // Help
    QMenu* helpMenu = new QMenu(tr("&Help"), this);
    menuBar()->addMenu(helpMenu);

    helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // save geometry
    QMainWindow::closeEvent(event);
}

void MainWindow::warmConfigure()
{
    std::cout << "warm configure" << std::endl;

    engine->addFactory(new UICoreFactory());
    panels = qobject_cast<QStackedWidget*>(engine->create("panels", "StackedView", true)->widget());
    splitterv->addWidget(panels);

    // QString file = QFileInfo("./dist/index.html").absoluteFilePath();
    // engine->loadHtmlFile(file, QUrl::fromLocalFile(file));

    if (!hostPath.isEmpty()) {
        engine->runFromUrl(QUrl("http://localhost:1234/index.html"));
    } else {
        engine->runFromUrl(QUrl::fromLocalFile(QFileInfo("./resources/index.html").absoluteFilePath()));
    }

    connect(engine, SIGNAL(engineReady()), this, SLOT(attachJSObjects()));
}

void MainWindow::attachJSObjects()
{
    engine->frame->addToJavaScriptWindowObject("app", &jsApp);

    QString keyBindingPath = QStandardPaths::locate(QStandardPaths::HomeLocation, ".ashlar", QStandardPaths::LocateDirectory);
    keyBindingPath += "/keybinding.json";

    QFile file(keyBindingPath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString script = "try { window.keyjson = " + file.readAll() + ";  } catch(err) { console.log(err) }";
        engine->runScript(script);
        engine->runScript("console.log(keyjson)");
        engine->runScript("setTimeout(()=>{keybinding.loadMap(keyjson)}, 250)");
    }
}

void MainWindow::emitEvent(QString event, QString payload)
{
}

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        // panels->hide();
        currentEditor()->editor->setFocus(Qt::ActiveWindowFocusReason);
        return;
    }

    if (!Commands::keyPressEvent(e)) {
        QMainWindow::keyPressEvent(e);
    }
}
