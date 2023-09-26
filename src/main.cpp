#include "mainwindow.h"

#include <QApplication>

qint32 main(qint32 argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
