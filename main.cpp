#include "reader.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowFlags(w.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    w.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    w.show();
    return a.exec();
}
