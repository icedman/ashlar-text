#include <QApplication>
#include <QCommandLineParser>

#include "json/json.h"
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);

    QCommandLineParser parser;
    QCommandLineOption inspectOption({ "i", "inspect" }, "show inspector");
    QCommandLineOption hostOption({ "x", "host" }, "development scripting host", "host", "");
    parser.addPositionalArgument("file", "file to edit");
    parser.addHelpOption();
    parser.addOption(inspectOption);
    parser.addOption(hostOption);
    parser.process(app);

    MainWindow window;

    if (parser.isSet(hostOption)) {
        window.setHost(parser.value(hostOption));
    }

    // window.resize(640, 512);
    window.resize(1200, 700);

    if (argc > 1) {
        window.openFile(argv[argc - 1]);
    } else {
        window.newFile();
    }

    window.show();

    if (parser.isSet(inspectOption)) {
        window.js()->showInspector(false);
    }

    return app.exec();
}