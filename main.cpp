#include "reader.h"
#include <QtWidgets/QApplication>
#include "QuaZip-Qt6-1.5/quazip/quazip.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Reader w;
    w.show();
    return a.exec();
}
