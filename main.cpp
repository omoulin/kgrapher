#include <QApplication>
#include <QCommandLineParser>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("kgrapher");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("KDE");

    QCommandLineParser parser;
    parser.setApplicationDescription("Function and surface grapher with 2D and 3D views");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    MainWindow *window = new MainWindow();
    window->show();

    return app.exec();
}
