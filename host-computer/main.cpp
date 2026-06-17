#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("传感器上位机");
    app.setApplicationVersion("1.0");

    MainWindow w;
    w.show();
    return app.exec();
}
