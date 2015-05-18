#include "easyNetMainWindow.h"
#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    QFont largerFont("Georgia", 12);
    QFont globalFont("Georgia", 10);
    a.setFont(globalFont);
    EasyNetMainWindow w;
    w.showMaximized();

    return a.exec();
}
