#include "networkemulator.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NetworkEmulator w;
    w.show();
    return a.exec();
}
