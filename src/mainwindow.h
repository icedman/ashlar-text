#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "editor.h"
#include "extension.h"
#include "icons.h"
#include "json/json.h"
#include "theme.h"

#include "js.h"
#include "qt/engine.h"

Q_DECLARE_METATYPE(Editor*)

class Tabs;
class Sidebar;
class Select;
class QStackedWidget;
class QSplitter;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

public slots:
    void about();
    void newFile(const QString& path = QString());
    void saveFile(bool saveNew = false);
    void saveFileAs();
    void openFile(const QString& path = QString());

public:
    void loadTheme(const QString& name);
    void configure();
    void setupMenu();
    void setupLayout();
    void applyTheme();
    void applySettings();
    void setHost(QString host);

    Editor* openTab(const QString& path = QString());
    int currentTab();

    void readSavedGeometry();
    void showCommandPalette();

    void emitEvent(QString event, QString payload);
    Engine* js() { return engine; }

    Editor* createEditor();
    Editor* currentEditor();
    Editor* findEditor(QString path);
    QStringList editorsPath();
    QStringList allFiles();

    bool loadExtension(QString name);

    static MainWindow* instance();

    QSplitter* horizontalSplitter() { return splitter; }
    QSplitter* verticalSplitter() { return splitterv; }

public:
    std::vector<Extension> extensions;
    theme_ptr theme;
    icon_theme_ptr icons;

    editor_settings_ptr editor_settings;
    Json::Value settings;

    QString projectPath;

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

public slots:
    void warmConfigure();
    void tabSelected(int index);
    void tabClose(int index);
    void loadAllExtensions();

private Q_SLOTS:
    void attachJSObjects();

private:
    QMenu* fileMenu;
    QMenu* viewMenu;

    QSplitter* splitter;
    QSplitter* splitterv;
    QStackedWidget* editors;
    QStackedWidget* panels;
    QStatusBar* statusbar;
    Tabs* tabs;
    Sidebar* sidebar;
    Select* select;

    QTimer updateTimer;

    JSFs jsFs;
    JSApp jsApp;
    Engine* engine;

    QString hostPath;
};

#endif // MAINWINDOW_H