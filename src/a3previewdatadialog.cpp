#include "incl/a3previewdatadialog.h"
#include "ui/ui_a3previewdatadialog.h"
#include "incl/savetoexcelfile.h"

#include "incl/a3database.h"

#include <QKeyEvent>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QtDebug>



A3PreviewDataDialog::A3PreviewDataDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::A3PreviewDataDialog)
{
    // Filter Kolom //
    {
        filterKolom <<
" exportname || (jkertas * jkopi * sisi) LIKE \"%%1%\" ";
        filterKolom << " klien LIKE \"%%1%\"";
        filterKolom << " file LIKE \"%%1%\"";
        filterKolom << " bahan LIKE \"%%1%\"";
        filterKolom << " jkertas LIKE \"%%1%\"";
        filterKolom << " jkopi LIKE \"%%1%\"";
        filterKolom << " keterangan LIKE \"%%1%\"";
        filterKolom << " (jkertas * jkopi * sisi) LIKE \"%%1%\"";
    }

    ui->setupUi(this);
    pm = new PreviewModel(this);
    ui->mainTable->setModel(pm);
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


    pm->setTable("a3pdata");
    pm->setLimit(ui->cbRow->currentText());
    pm->select();

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
    // ui->mainTable->viewport()->installEventFilter(this);
    ui->mainTable->installEventFilter(this);
    ui->mainTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mainTable, &QTableView::customContextMenuRequested, this, &A3PreviewDataDialog::mainTableContextMenu);
    fillDateFilter();
    manageNav();

    connect(ui->cbKolom, &QComboBox::currentTextChanged, this, &A3PreviewDataDialog::reload);
    connect(ui->cbTanggal, &QComboBox::currentTextChanged, this, &A3PreviewDataDialog::reload);
    connect(ui->tbKolomFilter, &QToolButton::clicked, this, &A3PreviewDataDialog::reload);
    connect(ui->cbRow, SIGNAL(currentTextChanged(QString)), pm, SLOT(setLimit(QString)));
    connect(ui->cbRow, &QComboBox::currentTextChanged, this, &A3PreviewDataDialog::reload);
    connect(ui->tbSave, &QToolButton::clicked, pm, &PreviewModel::submitAll);
    connect(ui->tbClose, &QToolButton::clicked, this, &A3PreviewDataDialog::close);
    connect(pm, SIGNAL(pageChanged()), this, SLOT(reload()));
    connect(pm, SIGNAL(limitChanged()), this, SLOT(reload()));

}

bool A3PreviewDataDialog::eventFilter(QObject *watched, QEvent *event)
{
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
    // qDebug() << generatedFilter();
    pm->setFilter(generatedFilter());
    manageNav();
}

void A3PreviewDataDialog::manageNav()
{
    QSqlQuery q;
    qint64 rowCount;
    int curPage;
    QString query("SELECT COUNT(*) FROM a3pdata WHERE "),
            currentFilter = pm->filter();
    q.exec(!currentFilter.isEmpty() ? query + currentFilter : "SELECT COUNT(*) FROM a3pdata");
    rowCount = q.next() ? q.value(0).toLongLong() : 0;
//    if(!rowCount)
//        qDebug() << q.lastError();
    curPage = pm->currentPage() + 1;
    lastMaxPage = qCeil((double) rowCount / pm->currentLimit());
    ui->tbFirst->setEnabled(curPage != 1);
    ui->tbPrev->setEnabled(curPage > 1);
    ui->lPaging->setText(QString("Page %1/%2").arg(curPage).arg(lastMaxPage));
    ui->tbNext->setEnabled(curPage < lastMaxPage);
    ui->tbLast->setEnabled(curPage != lastMaxPage);
}

QString A3PreviewDataDialog::generatedFilter() const
{
    int ifilter = ui->cbKolom->currentIndex();
    QString fltDate = ui->cbTanggal->currentText();
    QString fltText = ui->leKolomFilter->text().simplified();
    QString generated = fltText.isEmpty() ? "" : filterKolom[ifilter].arg(fltText);
    QStringList filters;
    if(!generated.isEmpty())
        filters << generated;
    if(fltDate != "Semua")
        filters << QString("DATE(created) = \"%1\"").arg(fltDate);
    return filters.join(" and ");
}

void A3PreviewDataDialog::fillDateFilter()
{
    QSqlQuery q("SELECT DISTINCT DATE(created) FROM a3pdata ORDER BY created DESC");
    ui->cbTanggal->clear();
    ui->cbTanggal->addItem("Semua");
    while(q.next())
        ui->cbTanggal->addItem(q.value(0).toString());
}

void A3PreviewDataDialog::mainTableContextMenu(const QPoint &pos)
{
    auto selMod = ui->mainTable->selectionModel();
//    if(!selMod->hasSelection())
//        return ;
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
                /*
                qDebug() << "insert row " << pm->insertRow(insrow);
                pm->setData(pm->index(insrow, 1), QDateTime::currentDateTime());
                pm->setData(pm->index(insrow, 2), tz["klien"]);
                pm->setData(pm->index(insrow, 3), tz["file"]);
                pm->setData(pm->index(insrow, 4), tz["bahan"]);
                pm->setData(pm->index(insrow, 5), tz["jkertas"]);
                pm->setData(pm->index(insrow, 6), tz["jkopi"]);
                pm->setData(pm->index(insrow, 7), tz["sisi"]);
                pm->setData(pm->index(insrow, 8), tz["keterangan"]);
                pm->setData(pm->index(insrow, 9), tz["exportName"]);
                */
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

void A3PreviewDataDialog::on_tbFirst_clicked()
{
    pm->setDisplayPage(0);
}

void A3PreviewDataDialog::on_tbPrev_clicked()
{
    pm->setDisplayPage(pm->currentPage() - 1);
}

void A3PreviewDataDialog::on_tbNext_clicked()
{
    pm->setDisplayPage(pm->currentPage() + 1);
}

void A3PreviewDataDialog::on_tbLast_clicked()
{
    pm->setDisplayPage(lastMaxPage - 1);
}

void A3PreviewDataDialog::on_tbPrint_clicked()
{
    saveToExcelFile(ui->mainTable);
}

