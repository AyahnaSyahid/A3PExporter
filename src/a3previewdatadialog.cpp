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
  
  ui->mainTable->setModel(pm);
  ui->mainTable->hideColumn(0);
  ui->mainTable->hideColumn(1);

  pm->setHeaderData(2, Qt::Horizontal, "Konsumen");
  pm->setHeaderData(3, Qt::Horizontal, "File");
  pm->setHeaderData(4, Qt::Horizontal, "Bahan");
  pm->setHeaderData(5, Qt::Horizontal, "Page/Halaman");
  pm->setHeaderData(6, Qt::Horizontal, "Ripit");
  ui->mainTable->hideColumn(7);
  pm->setHeaderData(7, Qt::Horizontal, "Sisi");
  pm->setHeaderData(8, Qt::Horizontal, "Keterangan");
  ui->mainTable->hideColumn(9);
  ui->mainTable->hideColumn(10);

  ui->mainTable->resizeColumnsToContents();
  // init cbTanggal
  initDateFilter();

  ui->cbTanggal->setCurrentIndex(0);
  
  // filter Copy table data as text
  ui->mainTable->installEventFilter(this);
  ui->mainTable->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->mainTable, &QTableView::customContextMenuRequested, this, &A3PreviewDataDialog::mainTableContextMenu);
  connect(ui->cbKolom, &QComboBox::currentIndexChanged, [=](int i){
    auto model = findChild<PreviewModel*>();
    switch (i) {
      case 1:
        model->setProperty("filterBy", "filterKlien");
        break;
      case 2:
        model->setProperty("filterBy", "filterFile");
        break;
      case 3:
        model->setProperty("filterBy", "filterBahan");
        break;
      case 4:
        model->setProperty("filterBy", "filterPage");
        break;
      case 5:
        model->setProperty("filterBy", "filterCopy");
        break;
      case 5:
        model->setProperty("filterBy", "filterInfo");
        break;
      case 6:
        model->setProperty("filterBy", "filterExportName");
        break;
      default:
        model->setProperty("filterBy", "all");
    }
    model->updateQuery();
  });

  connect(ui->cbTanggal, &QComboBox::currentIndexChanged, [=](int i) {
    auto model = findChild<PreviewModel*>();
    QString va = ui->cbTanggal->itemData(i).toString() == "all" ? "%" : ui->cbTanggal->itemData(i).toString();
    model->setProperty("filterDate", va);
    model->updateQuery();
  });

  connect(ui->cbRow, &QComboBox::currentIndexChanged, [=](int i) {
    auto model = findChild<PreviewModel*>();
    
  });
  connect(ui->tbClose, &QToolButton::clicked, this, &A3PreviewDataDialog::close);
  connect(ui->leKolomFilter, &QLineEdit::textChanged, [filterModel](QString s) {
      filterModel->setFilterFixedString(s); });
  connect(ui->leKolomFilter, &QLineEdit::textChanged, this, &A3PreviewDataDialog::manageNav, Qt::QueuedConnection);
  connect(filterModel, SIGNAL(pageChanged(int, int)), this, SLOT(manageNav()));
  connect(filterModel, SIGNAL(perPageChanged(int)), this, SLOT(manageNav()));
  connect(filterModel, SIGNAL(filterKeyChanged(int)), this, SLOT(manageNav()));
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

void A3PreviewDataDialog::reload()
{
    // pass
}

void A3PreviewDataDialog::manageNav()
{
    QSqlTableModel* pmod = findChild<QSqlTableModel*>();
    SortFilterModel* smod = findChild<SortFilterModel*>();
    
    int cp = smod->currentPage() + 1;
    int mp = smod->lastPage() + 1;
    
    ui->tbFirst->setEnabled(cp != 1);
    ui->tbPrev->setEnabled(cp > 1);
    ui->lPaging->setText(QString("Page %1/%2").arg(cp).arg(mp));
    ui->tbNext->setEnabled(cp < mp);
    ui->tbLast->setEnabled(cp != mp);
    
    qDebug() << "Curent Page / Max :" << cp << "/" << mp; 
}

void A3PreviewDataDialog::applyDateFilter()
{
    QSqlTableModel* pmod = findChild<QSqlTableModel*>();
    if(ui->cbTanggal->currentIndex() != 0) {
        QString dateFilter = ui->cbTanggal->currentText();
        pmod->setFilter(QString("DATE(created) = '%1'").arg(dateFilter));
    } else {
        pmod->setFilter("");
    }
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
    connect(delAct, &QAction::triggered, pm, [=](){
        // pm->removeRows(rows.first(), rows.count());
        for( int c : rows )
        {
            pm->removeRow(c);
        }
    });
    connect(addAct, &QAction::triggered, pm, [=](){
        int rAt = rows.last() + 1;
        pm->insertRow(rAt);
    });
    connect(picFromFile, &QAction::triggered, pm, [=](){
        A3DataBase ab;
        QStringList fname = QFileDialog::getOpenFileNames(this, "Pilih file", "", "PDF File (*.pdf)");
        if(fname.isEmpty())
            return;
        int insrow = selMod->hasSelection() ? selMod->selectedIndexes().first().row() : -1;
        for(const QString &rs : qAsConst(fname)){
            // ab.insert(rs);
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
        // reload();
    });
    contxMenu->exec();
    contxMenu->deleteLater();
}

void A3PreviewDataDialog::initDateFilter() {
    QSqlQuery q("SELECT DISTINCT DATE(created) FROM a3pdata ORDER BY created DESC");
    ui->cbTanggal->clear();
    while(q.next())
        ui->cbTanggal->insertItem(-1, q.value(0).toString(), q.value(0));
    ui->cbTanggal->insertItem(-1, "Semua", "all");
}

void A3PreviewDataDialog::on_tbFirst_clicked()
{
    SortFilterModel* sm = findChild<SortFilterModel*>();
    sm->setPage(0);
}

void A3PreviewDataDialog::on_tbPrev_clicked()
{
    SortFilterModel* sm = findChild<SortFilterModel*>();
    sm->setPage(sm->currentPage() - 1);
}

void A3PreviewDataDialog::on_tbNext_clicked()
{
    SortFilterModel* sm = findChild<SortFilterModel*>();
    sm->setPage(sm->currentPage() + 1);
}

void A3PreviewDataDialog::on_tbLast_clicked()
{
    SortFilterModel* sm = findChild<SortFilterModel*>();
    sm->setPage(sm->lastPage());
}

void A3PreviewDataDialog::on_tbPrint_clicked()
{
    saveToExcelFile(ui->mainTable);
}

void A3PreviewDataDialog::changeColumnFilter() {
    SortFilterModel* sm = findChild<SortFilterModel*>();
    switch (ui->cbKolom->currentIndex()) {
        case 1:
            sm->setFilterKeyColumn(2);
            break;
        case 2:
            sm->setFilterKeyColumn(3);
            break;
        case 3:
            sm->setFilterKeyColumn(4);
            break;
        case 4:
            sm->setFilterKeyColumn(5);
            break;
        case 5:
            sm->setFilterKeyColumn(6);
            break;
        case 6:
            sm->setFilterKeyColumn(7);
            break;
        default :
            sm->setFilterKeyColumn(-1);
    }
}