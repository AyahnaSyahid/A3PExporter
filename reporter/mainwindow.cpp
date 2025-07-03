#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "reportermodel.h"
#include "listfilenamemodel.h"
#include "printpreviewmodel.h"
#include "dirscanner.h"

#include "xlsxformat.h"
#include "xlsxdocument.h"
#include "xlsxcellrange.h"
#include "xlsxcell.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QIdentityProxyModel>
#include <QDateTime>
#include <QItemSelectionModel>
#include <QSettings>
#include <QInputDialog>

#include <QtDebug>

class ExcelWriter
{
  public:
  QXlsx::Document *document;
};

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setting = new QSettings("conf.ini", QSettings::IniFormat, this);
  if(!setting->contains("NamaPerusahaan"))
  {
    bool ret;
    QString perc = QInputDialog::getText(
      this,
      "Setel Nama Perusahaan",
      "Nama :",
      QLineEdit::Normal,
      "Bawaan",
      &ret
    );
    if(ret)
    {
      setting->setValue("NamaPerusahaan", perc);
    }
    else
    {
      setting->setValue("NamaPerusahaan", "?");
    }
    setting->sync();
  }
  QString ns = setting->value("NamaPerusahaan").toString();
  QString lastExportFolder = setting->value("Exporter/lastExportFolder").toString();
  if(!lastExportFolder.isEmpty()) {
    _ed = lastExportFolder;
  }
  ui->lineEdit_3->setText(_ed);
  ui->label->setText("*Tulisan merah berarti nama file tidak dimengerti oleh aplikasi");
  QFont lf = ui->label->font();
  lf.setItalic(true);
  // lf.setPixelSize(lf.pixelSize() - 3);
  ui->label->setStyleSheet("color: red");
  ui->label->setFont(lf);
  ui->lineEdit->hide();
  laporan = new ExcelWriter;
  setWindowTitle(QString("%1 | A3+ Job Viewer").arg(ns));
  setWindowIcon(QIcon(":/images/PikPng.com_blue-circle-png_1191296.png"));
  ReporterModel *repMod = new ReporterModel(this, &modelData);
  repMod->setObjectName("repMod");
  ui->tableView->setModel(repMod->getPreviewModel());
  ui->listView->setModel(repMod->getPreviewModel());
  ui->listView->resize(200, 300);
  for(int i = 0; i < repMod->columnCount(); i++)
  {
    ui->listView->hideColumn(i);
    ui->tableView->hideColumn(i);
  }
  ui->listView->showColumn(9);
  for(int i = 1; i < 9; ++i)
  ui->tableView->showColumn(i);
  
  ui->listView->verticalHeader()->hide();
  ui->listView->verticalHeader()->setMinimumSectionSize(15);
  ui->listView->verticalHeader()->setDefaultSectionSize(18);
  ui->listView->horizontalHeader()->setStretchLastSection(true);
  ui->listView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->listView->setSortingEnabled(true);
  ui->tableView->horizontalHeader()->setStretchLastSection(true);
  ui->tableView->setSortingEnabled(true);
  ui->tableView->horizontalHeader()->setSectionsMovable(true);
  ui->tableView->verticalHeader()->setMinimumSectionSize(15);
  ui->tableView->verticalHeader()->setDefaultSectionSize(18);
  ui->tableView->horizontalHeader()->setMinimumSectionSize(20);
  ui->tableView->horizontalHeader()->setDefaultSectionSize(50);
  ui->tableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
  
  
  QLabel *statCount = new QLabel("Jumlah File : 0 | Total Klik : 0 ");
  statCount->setObjectName("stCount");
  statCount->setProperty("templateString", "Jumlah File : %1 | Total Klik : %2");
  ui->statusbar->addPermanentWidget(statCount);
  
  ui->splitter->setSizes({180, 500});
  connect(this, &MainWindow::modelDataChanged, repMod, &ReporterModel::reset);
  connect(this, &MainWindow::modelDataChanged, this, &MainWindow::updateStBarSum);
  connect(this, &MainWindow::modelDataChanged, this,
    [=]()
    {
      ui->tableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
      ui->listView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    }
  );
  connect(this, &MainWindow::execDirChanged, ui->lineEdit_3, &QLineEdit::setText);
  connect(ui->listView->selectionModel(),
    &QItemSelectionModel::currentRowChanged,
    this,
    [=]()
    {
      QModelIndex idx = ui->listView->selectionModel()->currentIndex();
      ui->tableView->selectRow(idx.row());
    }
  );
  connect(ui->lineEdit_2, &QLineEdit::textChanged, qobject_cast<PrintPreviewModel*>(ui->tableView->model()), &PrintPreviewModel::setFilterWildcard);
  connect(ui->lineEdit_2, &QLineEdit::textChanged, this, &MainWindow::updateStBarSum);
  connect(repMod, &ReporterModel::dataChanged, this, &MainWindow::updateStBarSum);
  connect(repMod, &ReporterModel::rowsInserted, this, &MainWindow::updateStBarSum);
  connect(repMod, &ReporterModel::rowsRemoved, this, &MainWindow::updateStBarSum);
}
MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::setExecDir(const QString &ed)
{
  if(execDir() != ed)
  {
    _ed = ed;
    emit execDirChanged(ed);
  }
}

void MainWindow::updateStBarSum()
{
  PrintPreviewModel *model = static_cast<PrintPreviewModel*>(ui->tableView->model());
  int msum = 0;
  for(int r = 0; r < model->rowCount(); ++r)
  msum += model->data(model->index(r, 8), Qt::EditRole).toInt();
  QLabel *stBarLabel = ui->statusbar->findChild<QLabel*>("stCount");
  QString tpl = stBarLabel->property("templateString").toString();
  stBarLabel->setText(tpl.arg(model->rowCount()).arg(msum));
}
void MainWindow::on_pilihBtn_released()
{
  if(_ed.isEmpty())
    _ed = QApplication::applicationDirPath();
  QString ed = QFileDialog::getExistingDirectory(this, tr("Pilih direktori untuk di scan"), _ed);
  setExecDir(ed);
}

void MainWindow::on_mulaiBtn_released()
{
  ui->mulaiBtn->setDisabled(true);
  DirScanner *ds = new DirScanner;
  ds->setScanDir(_ed);
  connect(ds, &DirScanner::sendMessages, this, &MainWindow::writeStatus);
  connect(ds, &DirScanner::result, this, &MainWindow::scanDone);
  connect(ds, &DirScanner::finished, ds, &QObject::deleteLater);
  connect(ds, &DirScanner::finished, this, [=](){ui->mulaiBtn->setEnabled(true);});
  ds->start();
}

void MainWindow::scanDone(QList<QMap<QString, QVariant>> result)
{
  modelData = result;
  emit modelDataChanged();
}

void MainWindow::writeStatus(const QString &status)
{
  ui->statusbar->showMessage(status, 2000);
}

void MainWindow::on_listView_customContextMenuRequested(const QPoint &pos)
{
  QMenu contextMenu(ui->listView);
  QAction delRow, insRow;
  delRow.setText("Hapus");
  insRow.setText("Tambahkan");
  connect(&delRow, &QAction::triggered, this,
    [=]()
    {
      auto model = ui->listView->model();
      auto smodel = ui->listView->selectionModel();
      int rowAtPoint = ui->listView->rowAt(pos.y());
      
      if(smodel->selectedIndexes().size() > 1)
      {
        int rmin = INT32_MAX, rmax = 0, temp;
        QModelIndexList ilist = smodel->selectedIndexes();
        QModelIndexList::const_iterator it = ilist.constBegin();
        while(it != ilist.constEnd())
        {
          temp = it->row();
          rmin = temp < rmin ? temp : rmin;
          rmax = temp > rmax ? temp : rmax;
          it++;
        }
        model->removeRows(rmin, 1 + rmax - rmin);
        return ;
      }
      model->removeRow(rowAtPoint);
    });
    connect(&insRow, &QAction::triggered, this,
      [=]()
      {
        auto model = ui->listView->model();
        int rowAt = ui->listView->rowAt(pos.y());
        model->insertRow(rowAt);
      });
      contextMenu.addAction(&delRow);
      contextMenu.addAction(&insRow);
      contextMenu.exec(ui->listView->viewport()->mapToGlobal(pos));
}

void MainWindow::on_simpanBtn_released()
{
  QString currentDateStr = QDateTime::currentDateTime().toString("dddd, dd MMMM yyyy");
  laporan->document = new QXlsx::Document();
  auto _lap = laporan->document;
  auto prepMod = this->findChild<ReporterModel*>("repMod")->getPreviewModel();
  int rowCount = prepMod->rowCount(), claprow = 1;
  QXlsx::Format titleFmt, hdrFmt, tglFmt;
  auto writeRow = [&_lap](int r, QVariantList va, QList<QXlsx::Format> fmt = {})
  {
    
    
    int col = 0, vacount = va.size();
    for(; col < vacount; col++)
    {
      if(fmt.size() == vacount)
      _lap->write(r, col + 1, va.value(col), fmt.at(col));
      else if(fmt.size() > 0)
      _lap->write(r, col + 1, va.value(col), fmt.at(0));
      else
      _lap->write(r, col + 1, va.value(col));
    }
  };
  titleFmt.setHorizontalAlignment(titleFmt.AlignHCenter);
  titleFmt.setVerticalAlignment(titleFmt.AlignVCenter);
  titleFmt.setFontSize(22);
  QString dfn = titleFmt.fontName();
  titleFmt.setFontName("Times New Roman");
  titleFmt.setPatternBackgroundColor(QColor(255,255,160));
  titleFmt.setFontBold(true);
  
  QString namaPerc = setting->value("NamaPerusahaan", QVariant()).toString();
  _lap->write(claprow, 1, QString("LAPORAN A3 PLUS %1").arg(namaPerc.toUpper()));
  claprow++;
  _lap->mergeCells("A1:I1", titleFmt);
  titleFmt.setFontName(dfn);
  titleFmt.setHorizontalAlignment(titleFmt.AlignRight);
  titleFmt.setFontSize(11);
  titleFmt.setPatternBackgroundColor(Qt::black);
  titleFmt.setFontColor(Qt::white);
  titleFmt.setFontItalic(true);
  _lap->write(claprow, 7, currentDateStr, titleFmt);
  claprow++;
  _lap->mergeCells("G2:I2");
  
  hdrFmt.setFontBold(true);
  hdrFmt.setPatternBackgroundColor(Qt::lightGray);
  hdrFmt.setVerticalAlignment(hdrFmt.AlignVCenter);
  hdrFmt.setHorizontalAlignment(hdrFmt.AlignHCenter);
  hdrFmt.setBorderStyle(hdrFmt.BorderThin);
  hdrFmt.setBorderColor(Qt::black);
  writeRow(claprow,
    QVariantList() << "No" << "Konsumen" << "File" << "Bahan" << "Keterangan" << "Halaman" << "Banyak" << "Sisi" << "Total",
  {hdrFmt});
  claprow++;
  
  QXlsx::Format oddRow, evtrow;
  evtrow.setBorderStyle(evtrow.BorderThin);
  evtrow.setBorderColor(Qt::black);
  oddRow.setFontBold(true);
  oddRow.setPatternBackgroundColor(QColor(0xff, 0xdd, 0xdd));
  oddRow.setBorderStyle(oddRow.BorderThin);
  for(int rowData = 0; rowData < rowCount; ++rowData)
  {
    if(prepMod->rowData(rowData).at(1).isNull())
    continue;
    if(rowData % 2)
    writeRow(claprow, prepMod->rowData(rowData), {oddRow});
    else
    writeRow(claprow, prepMod->rowData(rowData), {evtrow});
    claprow++;
  }
  // fix width column
  
  _lap->setColumnWidth(1, 5.0);
  _lap->setColumnWidth(2, 20.0);
  _lap->setColumnWidth(3, 50.0);
  _lap->setColumnWidth(4, 14.0);
  _lap->setColumnWidth(5, 16.0);
  _lap->setColumnWidth(6, 10.0);
  _lap->setColumnWidth(7, 10.0);
  _lap->setColumnWidth(8, 10.0);
  _lap->setColumnWidth(9, 12.0);
  
  // fix function
  for(int n = 4; n < claprow; n++)
  {
    _lap->write(n, 9, QString("=F%1*G%1*H%1").arg(n));
  }
  // Total
  
  
  _lap->write(claprow, 7, "Jumlah", titleFmt);
  _lap->mergeCells(QString("G%1:H%1").arg(claprow));
  _lap->write(claprow, 9, QString("=SUM(I4:I%1)").arg(claprow-1), titleFmt);
  QString saveTo = QFileDialog::getSaveFileName(this,
    "Simpan Sebagai",
    ui->lineEdit_3->text() + " / " +
    currentDateStr + ".xlsx"
  );
  _lap->renameSheet("Sheet1", currentDateStr.remove(0, currentDateStr.indexOf(' ')));
  if(!_lap->saveAs(saveTo))
  QMessageBox::warning(this, "Gagal Menyimpan Berkas", "Silahkan coba lagi dalam beberapa tahun kedepan");
}

DirScanner::DirScanner(QObject* parent) : QThread(parent) {}
DirScanner::~DirScanner() {}

void DirScanner::run()
{
  QDir d(dirName);
  if(!d.exists())
  {
    emit errorOcurs(tr("Direktori tidak ditemukan"));
    return ;
  }
  custom_type tdata;
  QFileInfoList qfil;
  QDirIterator dit(dirName, QStringList() << "*.pdf", QDir::Files, QDirIterator::Subdirectories);
  int fileCount = 0;
  QString tmpl("Found #%1 : %2");
  while(dit.hasNext())
  {
    dit.next();
    qfil << dit.fileInfo();
    emit sendMessages(tmpl.arg(++fileCount).arg(dit.fileName()));
  }
  auto finfo = qfil.constBegin();
  int fcount = 0;
  for(;finfo != qfil.constEnd(); ++finfo, ++fcount)
  {
    QMap<QString, QVariant> temp;
    QString fname = finfo->fileName();
    int lastDot = fname.lastIndexOf('.');
    QStringList splited = fname.remove(lastDot, 4).split('_');
    temp["0validity"] = false;
    temp["1userCol"] = QVariant(QVariant::String);
    temp["2fileCol"] = QVariant(QVariant::String);
    temp["3matCol"] = QVariant(QVariant::String);
    temp["4desCol"] = QVariant(QVariant::String);
    temp["5halCol"] = 1;
    temp["6cntCol"] = 1;
    temp["7sideCol"] = 1;
    if(splited.count() >= 4)
    {
      temp["0validity"] = true;
      temp["1userCol"] = splited[0];
      temp["2fileCol"] = splited[1];
      temp["3matCol"] = splited[2];
      if(splited[3].contains('@'))
      {
        QVariantList pxc;
        QStringList sl = splited[3].split('@');
        for(const QString &s : qAsConst(sl))
        pxc << QVariant(s);
        temp["5halCol"] = pxc[0].toInt();
        temp["6cntCol"] = pxc[1].toInt();
      }
      else
      {
        temp["5halCol"] = 1;
        temp["6cntCol"] = QVariant(splited[3]).toInt();
      }
      if(splited.count() > 4)
      {
        QStringList rest;
        for(int i = 4; i < splited.count(); ++i)
        {
          if(splited[i].toUpper() == "BB")
          temp["7sideCol"] = 2;
          rest << splited[i];
        }
        
        temp["4desCol"] = rest.join('-');
      }
    }
    temp["9fileName"] = finfo->fileName();
    temp["9filePath"] = finfo->filePath();
    temp["9gcreateTime"] = finfo->birthTime();
    temp["8sumCol"] = temp["5halCol"].toInt() * temp["7sideCol"].toInt() * temp["6cntCol"].toInt();
    emit sendMessages(QString("%1 file/s Scanned..").arg(fcount));
    tdata.append(temp);
  }
  qRegisterMetaType<custom_type>();
  emit result(tdata);
}

void DirScanner::setScanDir(const QString &dirname)
{
  dirName = dirname;
}