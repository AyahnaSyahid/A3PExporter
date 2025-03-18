#include "incl/tentangaplikasi.h"
#include "ui/ui_tentangaplikasi.h"

TentangAplikasi::TentangAplikasi(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TentangAplikasi)
{
    ui->setupUi(this);
    ui->label->setText(ui->label->text().replace("Using Qt5.12", "Using Qt%1").arg(qVersion()));
}

TentangAplikasi::~TentangAplikasi()
{
    delete ui;
}
