#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_reader.h"

class Reader : public QMainWindow
{
    Q_OBJECT

public:
    Reader(QWidget *parent = nullptr);
    ~Reader();

private:
    Ui::ReaderClass ui;
};
