#include <QApplication>
#include <QCommandLineParser>

#include "json/json.h"
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);

    QCommandLineParser parser;
    parser.process(app);
        
    MainWindow window;
    // window.resize(640, 512);
    window.resize(1200, 700);

    if (argc > 1) {
        window.openFile(argv[1]);
    } else {
        window.newFile();
    }

    window.show();

    return app.exec();
}