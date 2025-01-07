#include "incl/tentangaplikasi.h"
#include "ui/ui_tentangaplikasi.h"

TentangAplikasi::TentangAplikasi(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TentangAplikasi)
{
    ui->setupUi(this);
}

TentangAplikasi::~TentangAplikasi()
{
    delete ui;
}
