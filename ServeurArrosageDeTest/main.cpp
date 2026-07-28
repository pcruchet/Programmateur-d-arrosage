#include "serveurarrosagewidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ServeurArrosageWidget w;
    w.show();
    return a.exec();
}
