#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "editor.h"
#include "extension.h"
#include "icons.h"
#include "json/json.h"
#include "sidebar.h"
#include "tabs.h"
#include "theme.h"

#include "js.h"
#include "qt/engine.h"

Q_DECLARE_METATYPE(Editor*)

class QStackedWidget;
class QSplitter;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

public slots:
    void about();
    void newFile();
    void saveFile();
    void openFile(const QString& path = QString());

public:
    void loadTheme(const QString& name);
    void configure();
    void setupMenu();
    void setupLayout();
    void applyTheme();
    void applySettings();

    Editor* openTab(const QString& path = QString());

    void closeEvent(QCloseEvent* event);

    bool processKeys(QString keys);
    void emitEvent(QString event, QString payload);
    Engine* js() { return engine; }

    Editor* createEditor();
    Editor* currentEditor();
    Editor* findEditor(QString path);
    QStringList editorsPath();

    int currentTab() { return tabs->currentIndex(); }

    static MainWindow* instance();

public:
    std::vector<Extension> extensions;
    theme_ptr theme;
    icon_theme_ptr icons;

    editor_settings_ptr editor_settings;
    Json::Value settings;

    QString projectPath;
    
public:

    void keyPressEvent(QKeyEvent* e) override;
    
public slots:
    void warmConfigure();
    void tabSelected(int index);
    void tabClose(int index);

private Q_SLOTS:
    void attachJSObjects();

private:
    QMenu* fileMenu;
    QMenu* viewMenu;

    QSplitter* splitter;
    QSplitter* splitterv;
    QStackedWidget* editors;
    QStackedWidget* panels;
    Tabs* tabs;
    Sidebar* sidebar;

    QTimer updateTimer;

    JSApp jsApp;
    Engine *engine;
};

#endif // MAINWINDOW_H