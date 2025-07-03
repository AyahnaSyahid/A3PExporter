#include "incl/tentangaplikasi.h"
#include "ui/ui_tentangaplikasi.h"

#ifndef EXPORTER_VERSION
  #define EXPORTER_VERSION "UNSPECIFIED"
#endif

TentangAplikasi::TentangAplikasi(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TentangAplikasi)
{
    ui->setupUi(this);
    ui->label->setText(ui->label->text().arg(EXPORTER_VERSION));
    ui->label->setText(ui->label->text().replace("2023", "2025").replace("Using Qt5.12", "Using Qt%1").arg(qVersion()));
}

TentangAplikasi::~TentangAplikasi()
{
    delete ui;
}
