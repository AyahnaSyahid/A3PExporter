#include "incl/a3previewdatadialog.h"
#include "ui/ui_a3previewdatadialog.h"
#include "incl/savetoexcelfile.h"
#include "incl/a3database.h"
#include "incl/previewmodel.h"
// #include "incl/sortfiltermodel.h"

#include <QSqlTableModel>
#include <QSortFilterProxyModel>
#include <QKeyEvent>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMetaType>
#include <QtDebug>

A3PreviewDataDialog::A3PreviewDataDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::A3PreviewDataDialog)
{
  ui->setupUi(this);
  ui->mainTable->setSortingEnabled(true);
  ui->mainTable->horizontalHeader()->setStretchLastSection(true);
  ui->mainTable->setAlternatingRowColors(true);
  ui->mainTable->verticalHeader()->setMinimumSectionSize(10);
  ui->mainTable->verticalHeader()->setDefaultSectionSize(17);
  ui->mainTable->verticalHeader()->hide();
  // ui->mainTable->verticalHeader()->hide();

  // Warna Table
  QPalette p = ui->mainTable->palette();
  QColor base(225,255,225), alternate(125, 200, 125);
  p.setColor(QPalette::Base, base);
  p.setColor(QPalette::AlternateBase, alternate);
  ui->mainTable->setGridStyle(Qt::SolidLine);
  ui->mainTable->setPalette(p);
  // ui->mainTable->setStyleSheet("gridline-color: rgb(100, 100, 100)");

  PreviewModel* pm = new PreviewModel(this);
  QSortFilterProxyModel* proxy = new QSortFilterProxyModel(this);
  proxy->setSourceModel(pm);
  pm->updateQuery();
  // init cbTanggal
  initDateFilter();

  ui->cbTanggal->setCurrentIndex(0);
  
  // filter Copy table data as text
  ui->mainTable->installEventFilter(this);
  ui->mainTable->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->mainTable, &QTableView::customContextMenuRequested, this, &A3PreviewDataDialog::mainTableContextMenu);
  connect(ui->cbKolom, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int i){
    auto model = findChild<PreviewModel*>();
    switch (i) {
      case 1:
        model->setProperty("filterBy", "klien");
        break;
      case 2:
        model->setProperty("filterBy", "file");
        break;
      case 3:
        model->setProperty("filterBy", "bahan");
        break;
      case 4:
        model->setProperty("filterBy", "jkertas");
        break;
      case 5:
        model->setProperty("filterBy", "jkopi");
        break;
      case 6:
        model->setProperty("filterBy", "keterangan");
        break;
      case 7:
        model->setProperty("filterBy", "exportName");
        break;
      default:
        model->setProperty("filterBy", "all");
    }
    model->updateQuery();
  });

  connect(ui->cbTanggal, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int i) {
    auto model = findChild<PreviewModel*>();
    QString va = ui->cbTanggal->itemData(i).toString() == "all" ? "all" : ui->cbTanggal->itemData(i).toString();
    model->setProperty("filterDate", va);
    model->updateQuery();
  });

  connect(ui->cbRow, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int i) {
    auto model = findChild<PreviewModel*>();
    auto vardata = ui->cbRow->itemData(ui->cbRow->currentIndex(), Qt::DisplayRole);    
    int perPage = vardata.canConvert(QMetaType::Int) ? vardata.toInt() : 0;
    model->setProperty("rowLimit", perPage);
    model->updateQuery();
  });

  connect(ui->tbClose, &QToolButton::clicked, this, &A3PreviewDataDialog::close);
  connect(ui->leKolomFilter, &QLineEdit::textChanged, [=](QString s) {
    auto model = findChild<PreviewModel*>();
    model->setProperty("filterValue", s.isEmpty() ? "%" : QString("%%1%").arg(s));
    model->updateQuery();
      });

  connect(pm, SIGNAL(pageChanged(int, int)), this, SLOT(manageNav(int, int)));
  connect(ui->tbFirst, &QToolButton::clicked, pm, &PreviewModel::firstPage);
  connect(ui->tbPrev, &QToolButton::clicked, pm, &PreviewModel::prevPage);
  connect(ui->tbNext, &QToolButton::clicked, pm, &PreviewModel::nextPage);
  connect(ui->tbLast, &QToolButton::clicked, pm, &PreviewModel::lastPage);
  ui->mainTable->setModel(proxy);
  ui->mainTable->hideColumn(0);
  ui->mainTable->hideColumn(1);
  proxy->setHeaderData(2, Qt::Horizontal, "Konsumen");
  proxy->setHeaderData(3, Qt::Horizontal, "File");
  proxy->setHeaderData(4, Qt::Horizontal, "Bahan");
  proxy->setHeaderData(5, Qt::Horizontal, "Page/Halaman");
  proxy->setHeaderData(6, Qt::Horizontal, "Ripit");
  ui->mainTable->hideColumn(7);
  proxy->setHeaderData(7, Qt::Horizontal, "Sisi");
  proxy->setHeaderData(8, Qt::Horizontal, "Keterangan");
  ui->mainTable->hideColumn(9);
  ui->mainTable->hideColumn(10);
  ui->mainTable->resizeColumnsToContents();
  ui->lPaging->setText(QString("Page %1/%2").arg(1).arg(pm->maxPage()));
  ui->tbFirst->setDisabled(true);
  ui->tbPrev->setDisabled(true);
}

bool A3PreviewDataDialog::eventFilter(QObject *watched, QEvent *event)
{
    // implementasi CopyData CTRL+C
    if(watched == ui->mainTable)
    {
        if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if(ke->matches(QKeySequence::Copy))
            {
                auto model = ui->mainTable->selectionModel();
                QModelIndexList indexes = model->selectedIndexes();
                QString text;
                int row = indexes.first().row();
                for(auto current = indexes.begin(); current != indexes.end(); ++current)
                {
                    if(row == current->row())
                    {
                        text += current->data().toString() + ",";
                    } else {
                        row++;
                        if(text.endsWith(","))
                            text.chop(1);
                        text += "\n";
                        text += current->data().toString() + ",";
                    }
                }
                QClipboard *cb = QApplication::clipboard();
                if(text.endsWith(","))
                    text.chop(1);
                cb->setText(text);
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

A3PreviewDataDialog::~A3PreviewDataDialog()
{
    delete ui;
}

void A3PreviewDataDialog::manageNav(int cp, int mp)
{   
  ui->tbFirst->setEnabled(cp != 1);
  ui->tbPrev->setEnabled(cp > 1);
  ui->lPaging->setText(QString("Page %1/%2").arg(cp).arg(mp));
  ui->tbNext->setEnabled(cp < mp);
  ui->tbLast->setEnabled(cp != mp);  
}

void A3PreviewDataDialog::mainTableContextMenu(const QPoint &pos)
{
    auto selMod = ui->mainTable->selectionModel();

    auto selRowList = selMod->selectedIndexes();
    QList<int> rows;
    int crow = -1;
    Q_FOREACH(auto cidx, selRowList)
    {
        // qDebug() << cidx.row();
        if(cidx.row() != crow)
            rows << cidx.row();
        crow = cidx.row();
    }
    // qDebug() << rows;
    QMenu *contxMenu = new QMenu(ui->mainTable);
    QAction *delAct = new QAction("Hapus", contxMenu);
    QAction *addAct = new QAction("Tambahkan", contxMenu);
    addAct->setToolTip("Tambahkan data secara manual");
    QAction *picFromFile = new QAction("Tambahkan file");
    picFromFile->setToolTip("Tambahkan data dari file yang sudah ada");
    if(selMod->hasSelection()) contxMenu->addAction(delAct);
    contxMenu->addAction(addAct);
    contxMenu->addAction(picFromFile);
    contxMenu->move(ui->mainTable->mapToGlobal(pos));
    connect(delAct, &QAction::triggered, [=](){
        auto pm = findChild<PreviewModel*>();
        for( int c : rows )
        {
            pm->removeRow(c);
        }
    });
    connect(addAct, &QAction::triggered, [=](){
        auto pm = findChild<PreviewModel*>();
        int rAt = rows.last() + 1;
        pm->insertRow(rAt);
    });
    connect(picFromFile, &QAction::triggered, [=](){
        A3DataBase ab;
        auto pm = findChild<PreviewModel*>();
        QStringList fname = QFileDialog::getOpenFileNames(this, "Pilih file", "", "PDF File (*.pdf)");
        if(fname.isEmpty())
            return;
        int insrow = selMod->hasSelection() ? selMod->selectedIndexes().first().row() : -1;
        for(const QString &rs : qAsConst(fname)){
            QVariantMap tz = ab.fileStringToMap(rs);
            QSqlRecord r = pm->record();
            if(!tz.isEmpty())
            {
                r.setGenerated(1, false);
                r.setValue(2, tz["klien"]);
                r.setValue(3, tz["file"]);
                r.setValue(4, tz["bahan"]);
                r.setValue(5, tz["jkertas"]);
                r.setValue(6, tz["jkopi"]);
                r.setValue(7, tz["sisi"]);
                r.setValue(8, tz["keterangan"]);
                r.setValue(9, tz["exportName"]);
                // qDebug() << "insert record at" << insrow <<
                pm->insertRecord(insrow, r);
            }
        }
    });
    contxMenu->exec();
    contxMenu->deleteLater();
}

void A3PreviewDataDialog::initDateFilter() {
    QSqlQuery q("SELECT DISTINCT DATE(created) FROM a3pdata ORDER BY created ASC");
    ui->cbTanggal->clear();
    while(q.next())
        ui->cbTanggal->insertItem(-1, q.value(0).toString(), q.value(0));
    ui->cbTanggal->insertItem(-1, "Semua", "all");
}

void A3PreviewDataDialog::on_tbPrint_clicked()
{
    saveToExcelFile(ui->mainTable);
}