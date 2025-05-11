#include "reader.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Reader w;
    w.show();
    return a.exec();
}
