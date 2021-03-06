#include <QStackedWidget>
#include <QStandardPaths>
#include <QtWidgets>

#include <iostream>

#include "commands.h"
#include "icons.h"
#include "mainwindow.h"
#include "reader.h"
#include "settings.h"
#include "theme.h"

#include "select.h"
#include "sidebar.h"
#include "tabs.h"

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

    setWindowTitle(tr("Ashlar"));
    setMinimumSize(600, 400);

    updateTimer.singleShot(500, this, SLOT(warmConfigure()));
    connect(engine, SIGNAL(engineReady()), this, SLOT(attachJSObjects()));
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

QStringList MainWindow::allFiles()
{
    if (sidebar) {
        return sidebar->allFiles();
    }
    return QStringList();
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
    QString userSettings = QStandardPaths::locate(QStandardPaths::HomeLocation, ".ashlar", QStandardPaths::LocateDirectory) + "/settings.json";

    if (!QFile(userSettings).exists()) {
        QFile::copy(":/resources/settings.json", userSettings);
        QFile::setPermissions(userSettings, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadUser | QFileDevice::WriteUser);
    }

    load_settings(userSettings, settings);

    QString userExtensions = QStandardPaths::locate(QStandardPaths::HomeLocation, ".ashlar/extensions", QStandardPaths::LocateDirectory);
    load_extensions(userExtensions, extensions);

    if (settings.isMember("extensions_paths")) {
        Json::Value exts = settings["extensions_paths"];
        if (exts.isArray()) {
            for (auto path : exts) {
                qDebug() << path.asString().c_str();
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
    if (settings["default_icons"].isString()) {
        icons_default = icon_theme_from_name(settings["default_icons"].asString().c_str(), extensions);
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
    theme_application(theme, colors);

    // update all editors
    for (int i = 0; i < editors->count(); i++) {
        Editor* editor = (Editor*)editors->widget(i);
        editor->setTheme(theme);
    }

    // register icons
    engine->registerIcon("close", icon_for_file(icons_default, "close", "icon_close", extensions, colors.tabFg));
    engine->registerIcon("wrap", icon_for_file(icons_default, "wrap", "icon_wrap", extensions, colors.tabFg));
    engine->registerIcon("preview", icon_for_file(icons_default, "preview", "icon_preview", extensions, colors.tabFg));

    // reapply closeButton color
    closeButton->setIcon(engine->icon("close"));
}

void MainWindow::setHost(QString path)
{
    hostPath = path;
}

void MainWindow::applySettings()
{
    // editor settings
    editor_settings->mini_map = settings.isMember("mini_map") && settings["mini_map"] == true;
    editor_settings->gutter = settings.isMember("gutter") && settings["gutter"] == true;

    if (settings.isMember("font")) {
        strncpy(editor_settings->font, settings["font"].asString().c_str(), 32);
    } else {
        strcpy(editor_settings->font, "monospace");
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
    editor_settings->debug_scopes = settings.isMember("debug_scopes") && settings["debug_scopes"] == true;
    editor_settings->smooth_scroll = settings.isMember("smooth_scroll") && settings["smooth_scroll"] == true;

    // qDebug() << editor_settings->word_wrap;
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

    if (settings.isMember("sidebar") && settings["sidebar"] == true) {
        QFont font;
        font.setFamily(editor_settings->font);
        font.setPointSize(editor_settings->font_size);
        font.setFixedPitch(true);
        sidebar->setFont(font);
        if (sidebar->root() != "") {
            sidebar->animateShow();
        }
    } else {
        sidebar->hide();
    }

    if (settings.isMember("statusbar") == true) {
        QFont font;
        font.setFamily(editor_settings->font);
        font.setPointSize(editor_settings->font_size);
        font.setFixedPitch(true);
        statusBar()->setFont(font);
        statusBar()->show();
    } else {
        statusBar()->hide();
    }

    if (settings.isMember("menubar") == true) {
        menuBar()->show();
    } else {
        menuBar()->hide();
    }
}

void MainWindow::setupLayout()
{
    splitterv = new QSplitter(Qt::Vertical);
    splitter = new QSplitter(Qt::Horizontal);
    editors = new QStackedWidget();

    sidebar = new Sidebar(this);

    tabs = new Tabs(this);
    tabs->setDrawBase(false);
    tabs->setExpanding(false);
    tabs->setTabsClosable(false);

    connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(selectTab(int)));
    connect(tabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    QWidget* mainPane = new QWidget();
    QVBoxLayout* vbox = new QVBoxLayout();

    mainPane->setLayout(vbox);

    closeButton = new QPushButton(this);
    closeButton->setProperty("className", "closeButton");
    closeButton->setMaximumSize(28, 28);
    closeButton->setIconSize(QSize(12, 12));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeCurrentTab()));

    QHBoxLayout* thbox = new QHBoxLayout();
    thbox->addWidget(tabs);
    thbox->addWidget(closeButton);

    vbox->addLayout(thbox);
    vbox->addWidget(editors);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    splitterv->addWidget(splitter);
    splitterv->setStretchFactor(4, 1);

    splitter->addWidget(sidebar);
    splitter->addWidget(mainPane);
    splitter->setStretchFactor(1, 4);

    setCentralWidget(splitterv);

    select = new Select(this);
}

void MainWindow::newFile(const QString& path)
{
    int tabIdx = tabs->addTab(UNTITLED_TEXT);
    Editor* editor = createEditor();
    editors->addWidget(editor);
    tabs->setTabData(tabIdx, QVariant::fromValue(editor));
    selectTab(tabIdx);

    if (path.isEmpty()) {
        editor->setLanguage(language_from_file("untitled.txt", extensions));
    } else {
        editor->newFile(path);
        editor->setLanguage(language_from_file(path, extensions));
        tabs->setTabText(tabIdx, QFileInfo(path).fileName());
    }
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
        QString previousName = currentEditor()->fileName;
        if (currentEditor()->saveFile(QFileInfo(fileName).absoluteFilePath())) {
            tabs->setTabText(tabs->currentIndex(), QFileInfo(currentEditor()->fileName).fileName());
            statusBar()->showMessage("Saved " + currentEditor()->fileName, 5000);

            if (previousName != fileName) {
                currentEditor()->setLanguage(language_from_file(fileName, extensions));
                currentEditor()->openFile(fileName);
            }
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
                sidebar->setRootPath(projectPath, false, true);
            }
            newFile();
        }
        return;
    }

    if (QFile::exists(fileName)) {
        fileName = QFileInfo(fileName).absoluteFilePath();

        if (tabs->count() == 0) {
            projectPath = QFileInfo(fileName).path();
            sidebar->setRootPath(projectPath, true);
            sidebar->hide();
        }

        openTab(fileName);
        if (currentEditor()->fileName != fileName) {
            currentEditor()->setLanguage(language_from_file(fileName, extensions));
            currentEditor()->openFile(fileName);
            selectTab(tabs->findTabByPath(currentEditor()->fileName));
        }

        if (tabs->count() == 2) {
            // close untitled tab
            int idx = tabs->findTabByName(UNTITLED_TEXT);
            if (idx != -1) {
                Editor* e = tabs->editor(idx);
                if (e && !e->editor->document()->isUndoAvailable()) {
                    closeTab(idx);
                }
            }
        }

        return;
    } else {
        if (!tabs->count()) {
            newFile(QFileInfo(fileName).absoluteFilePath());
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

void MainWindow::selectTab(int index)
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
            sidebar->setActiveFile(_editor->fileName);

            emitEvent("tabSelected", _editor->fileName);
        }
    }
}

void MainWindow::closeCurrentTab()
{
    closeTab(currentTab());
}

void MainWindow::closeAllTabs()
{
    for (auto path : editorsPath()) {
        int idx = tabs->findTabByPath(path);
        closeTab(idx);
        if (tabs->findTabByPath(path) != -1) {
            // close cancelled
            break;
        }
    }
}

void MainWindow::closeTab(int index)
{
    bool untitledTab = false;

    // std::cout << "Close " << index << std::endl;

    if (index >= 0 && index < tabs->count()) {
        selectTab(index);
        untitledTab = (tabs->tabText(0) == UNTITLED_TEXT);
        QVariant data = tabs->tabData(index);
        Editor* _editor = qvariant_cast<Editor*>(data);
        if (_editor) {

            bool confirmClose = true;

            // if (_editor->editor->document()->isUndoAvailable()) {
            if (_editor->hasUnsavedChanges()) {
                // confirm
                QDialog confirm;
                confirm.setWindowTitle(" ");
                confirm.setModal(true);
                QVBoxLayout* v = new QVBoxLayout();
                QHBoxLayout* h = new QHBoxLayout();
                QPushButton* save = new QPushButton("Save");
                QPushButton* cancel = new QPushButton("Cancel");
                QPushButton* close = new QPushButton("Close without saving");

                confirm.setLayout(v);

                QString msgText = "<p><br/>Save changes made to <em>" + tabs->tabText(index) + "</em> before closing?<br/></p>";
                QLabel* msg = new QLabel(msgText);
                v->addWidget(msg);

                v->addLayout(h);
                h->addWidget(close);
                h->addWidget(cancel);
                h->addWidget(save);

                connect(close, SIGNAL(clicked()), &confirm, SLOT(accept()));
                connect(cancel, SIGNAL(clicked()), &confirm, SLOT(reject()));
                connect(save, SIGNAL(clicked()), this, SLOT(saveFile()));
                connect(save, SIGNAL(clicked()), &confirm, SLOT(accept()));

                confirm.exec();

                confirmClose = confirm.result() == QDialog::Accepted;
            }

            if (!confirmClose) {
                return;
            }

            emitEvent("tabClosed", _editor->fileName);
            editors->removeWidget(_editor);
            tabs->removeTab(index);
            _editor->deleteLater();
        }
    }

    if (!tabs->count()) {
        if (untitledTab) {
            close();
            return;
        }
        newFile();
        return;
    }

    selectTab(index);
}

Editor* MainWindow::openTab(const QString& _path)
{
    QString path = _path;
    QString fileName = QFileInfo(path).fileName();

    int tabInsertIndex = 0;
    int tabIdx = -1;
    for (int i = 0; i < tabs->count(); i++) {
        QVariant data = tabs->tabData(i);
        Editor* _editor = qvariant_cast<Editor*>(data);
        if (_editor->fileName == path) {
            tabIdx = i;
            break;
        }

        // insertion sort
        QString editorFileName = QFileInfo(_editor->fileName).fileName();
        if (!editorFileName.isEmpty()) {
            if (QFileInfo(editorFileName).fileName().compare(fileName) < 0) {
                tabInsertIndex = i + 1;
            }
        }
    }

    if (tabIdx != -1) {
        selectTab(tabIdx);
        return currentEditor();
    }

    QString _fileName = QFileInfo(path).fileName();
    if (_fileName.isEmpty()) {
        _fileName = UNTITLED_TEXT;
        path = QFileInfo(_fileName).absoluteFilePath();
    }

    int previewTab = tabs->findPreviewTab();
    if (previewTab != -1) {
        Editor* previewEditor = tabs->editor(previewTab);
        tabIdx = previewTab;
    }

    if (tabIdx == -1) {
        tabIdx = tabs->insertTab(tabInsertIndex, _fileName);
    }

    Editor* _editor = createEditor(); // << creates a new editor
    editors->addWidget(_editor);
    tabs->setTabData(tabIdx, QVariant::fromValue(_editor));
    tabs->setTabText(tabIdx, QFileInfo(_fileName).fileName());
    tabs->setTabIcon(tabIdx, engine->icon("preview"));
    tabs->setIconSize(QSize(12, 12));

    selectTab(tabIdx);

    return _editor;
}

int MainWindow::currentTab() { return tabs->currentIndex(); }

void MainWindow::setupMenu()
{
    // File
    fileMenu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(fileMenu);

    fileMenu->addAction(
        tr("&New"),
        this, [this]() { newFile(); }, QKeySequence::New);
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
        this, [this]() { if (sidebar->isVisible()) { sidebar->animateHide(); } else { sidebar->animateShow(); } });
    viewMenu->addAction(
        tr("Toggle Minimap"),
        this, [this]() { editor_settings->mini_map = !editor_settings->mini_map; currentEditor()->hide(); currentEditor()->show(); });
    viewMenu->addAction(
        tr("Toggle Statusbar"),
        this, [this]() { statusBar()->setVisible(!statusBar()->isVisible()); });
    // viewMenu->addAction(
    // tr("Show Command Palette"),
    // this, [this]() { select->setVisible(!select->isVisible()); }, QKeySequence("ctrl+p"));

    menuBar()->addMenu(viewMenu);

    // Help
    QMenu* helpMenu = new QMenu(tr("&Help"), this);
    menuBar()->addMenu(helpMenu);

    helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}

void MainWindow::readSavedGeometry()
{
}

void MainWindow::showCommandPalette()
{
    if (select) {
        select->show();
    }
}

void MainWindow::warmConfigure()
{
    std::cout << "warm configure" << std::endl;

    engine->addFactory(new UICoreFactory());
    panels = qobject_cast<QStackedWidget*>(engine->create("panels", "StackedView", true)->widget());
    statusbar = qobject_cast<QStatusBar*>(engine->create("statusBar", "StatusBar", true)->widget());
    statusbar->setSizeGripEnabled(false);
    setStatusBar(statusbar);

    select->setup();
    splitterv->addWidget(panels);

    if (!hostPath.isEmpty()) {
        engine->runFromUrl(QUrl(hostPath));
    } else {
        // load the main extension
        QString basePath = QCoreApplication::applicationDirPath();
        engine->runFromUrl(QUrl::fromLocalFile(QFileInfo(basePath + "/dist/index.html").absoluteFilePath()));
    }

    // reapply
    applySettings();
    applyTheme();
}

void MainWindow::attachJSObjects()
{
    engine->frame->addToJavaScriptWindowObject("app", &jsApp);
    engine->frame->addToJavaScriptWindowObject("fs", &jsFs);
    engine->frame->addToJavaScriptWindowObject("ps", &jsPs);
}

bool MainWindow::loadExtension(QString name)
{
    for (auto ext : extensions) {
        if (ext.name == name) {
            qDebug() << "extension loaded:" << name;
            engine->runScriptFile(ext.entryPath);
        }
    }
    return true;
}

void MainWindow::loadAllExtensions()
{
    // keybinding
    QString keyBindingPath = QStandardPaths::locate(QStandardPaths::HomeLocation, ".ashlar", QStandardPaths::LocateDirectory) + "/keybinding.json";

    if (!QFile(keyBindingPath).exists()) {
        QFile::copy(":/resources/keybinding.json", keyBindingPath);
        QFile::setPermissions(keyBindingPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadUser | QFileDevice::WriteUser);
    }
    
    QFile file(keyBindingPath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString script = "try { window.keyjson = " + file.readAll() + ";  } catch(err) { console.log(err) }";
        engine->runScript(script);
        engine->runScript("console.log(keyjson)");
        engine->runScript("setTimeout(()=>{keybinding.loadMap(keyjson)}, 250)");
    }

    // attach extensions
    settings.isMember("extensions");
    std::vector<std::string> keys = settings["extensions"].getMemberNames();
    std::vector<std::string>::iterator it = keys.begin();
    while (it != keys.end()) {
        QString name = (*it).c_str();
        loadExtension(name);
        QString script = "try { ashlar.extensions.activate('" + name + "');  } catch(err) { console.log(err) }";
        engine->runScript(script);
        it++;
    }
}

void MainWindow::emitEvent(QString event, QString payload)
{
    if (payload.isEmpty()) {
        payload = "\"\"";
    } else if (payload[0] != '{' && payload != '[') {
        payload = "\"" + payload + "\"";
    }

    // qDebug() << payload;

    engine->runScript("try { ashlar.events.emit(\"" + event + "\", " + payload + "); } catch(err) { console.log(err) } ");
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

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (tabs->count()) {
        closeAllTabs();
        if (tabs->count()) {
            event->setAccepted(false);
            return;
        }
    }

    js()->hideInspector();
    QMainWindow::closeEvent(event);
}
