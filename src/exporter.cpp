#include "incl/exporter.h"
#include "ui/ui_exporter.h"
#include "incl/a3previewdatadialog.h"
#include "incl/tentangaplikasi.h"
#include "incl/a3database.h"
// #include "incl/corelthread.h"
#include "corelmanager/incl/corelmanager.h"

#include <QCompleter>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSharedMemory>

#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMenu>
#include <QShortcut>

#include <QMessageBox>
#include <QCoreApplication>
#include <QtDebug>
#include <QScreen>


namespace SingletonNS {
    CorelManager* manager = nullptr;
}

int KalkulasiJumlahHalaman(const QString &s)
{
    int ctr = 0;
    QStringList sl = s.split(",",QString::SkipEmptyParts);
    for(auto part = sl.begin(); part != sl.end(); ++part)
    {
        auto temp = (*part).split("-");
        if(temp.count() == 1)
            ctr += 1;
        else
            ctr += 1 + temp.value(1).toInt() - temp.value(0).toInt();
    }

    return ctr ? ctr : -1;
}

Exporter::Exporter(QWidget *parent)
    : ui(new Ui::Exporter),
      db(new A3DataBase), 
      QWidget(parent)
{
    qsm = new QSharedMemory("ExporterAlive", this);
    // first attach
    if(qsm->create(8))
        firstInstance = true;
    else
        firstInstance = false;
    
    QDir().setCurrent(qApp->applicationDirPath());
    
    ui->setupUi(this);

    glb = new QSettings("conf.ini", QSettings::IniFormat, this);

    CorelManager::scan();
    
    CorelManager *corelManager = new CorelManager(this);

    if(!SingletonNS::manager) SingletonNS::manager = corelManager;
    
    corelManager->setObjectName("CorelManager");

    auto availableVersions = *CorelManager::installedVersions();

    ui->comboVersi->clear();

    if(!availableVersions.isEmpty()) {
        int idx = 0;
        for(auto i = availableVersions.constBegin();
            i != availableVersions.constEnd() ; ++i) {
            ui->comboVersi->addItem(QString("%1").arg(i.key()), i->first);
            ui->comboVersi->setItemData(idx++, i->second, Qt::UserRole + 1);
        }
    }
    
    QString corelVersion = glb->value("CorelApplication/useVersion").toString();
    auto lvu = ui->comboVersi->findText(corelVersion);
    ui->comboVersi->setCurrentIndex(lvu != -1 ? lvu : ui->comboVersi->currentIndex());
    auto *wk = corelManager->getWorker();
    
    connect(wk, &CorelWorker::result, this, &Exporter::handleCorelResult);
    connect(wk, &CorelWorker::exportMessage, this, &Exporter::handleExportResult);
    connect(wk, &CorelWorker::beginProcessing, this, &Exporter::disablesAll);
    connect(wk, &CorelWorker::endProcessing, this, &Exporter::enablesAll);
    connect(ui->comboVersi, SIGNAL(currentIndexChanged(int)), this, SLOT(comboVersiChanged(int)));


    ui->leExpF->setText(glb->value("Exporter/lastExportFolder").toString());

    QRegularExpression reQty("^[1-9]\\d*(@[1-9]\\d*)?");
    auto *validator = new QRegularExpressionValidator(reQty, ui->leQty);
    validator->setObjectName("validatorQty");
    ui->leQty->setValidator(validator);

    QRegularExpression rePage("^([1-9]\\d*)((-[1-9]\\d*)|(,[1-9]\\d*))*$");
    auto *pageVal = new QRegularExpressionValidator(rePage, ui->lePage);
    pageVal->setObjectName("validatorPage");
    ui->lePage->setValidator(pageVal);

    ui->kurvaOto->setChecked(true);
    connect(ui->lePage, &QLineEdit::textChanged, this, &Exporter::updateQty);
    connect(ui->lePage, &QLineEdit::editingFinished, this,
            [=]()
    {
        if(ui->lePage->text().endsWith("-") | ui->lePage->text().endsWith(","))
            ui->lePage->setText(ui->lePage->text().chopped(1));
    }
    );

    connect(ui->sideCheck, &QCheckBox::toggled, this, &Exporter::updateQty);

    CustomClassNS::FSCompleter *fileCompl = new CustomClassNS::FSCompleter;
    // QCompleter *fileCompl = new QCompleter();

    CustomClassNS::FileSystemModel *fsmodel = new CustomClassNS::FileSystemModel;
    // QFileSystemModel *fsmodel = new QFileSystemModel();

    fsmodel->setFilter(QDir::Dirs | QDir::Drives | QDir::AllDirs | QDir::NoDotAndDotDot);
    fsmodel->setRootPath("");
    fileCompl->setModel(fsmodel);
    fileCompl->setCompletionRole(Qt::DisplayRole);

    ui->leExpF->setCompleter(fileCompl);
    fileCompl->setCompletionMode(QCompleter::PopupCompletion);
    fileCompl->setCaseSensitivity(Qt::CaseInsensitive);
    fileCompl->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

    fsmodel->setParent(ui->leExpF);
    fileCompl->setParent(ui->leExpF);

    connect(this, &Exporter::exportFolderChanged, ui->leExpF, &QLineEdit::setText);
    connect(ui->leExpF, &QLineEdit::textChanged, ui->leExpF, &QLineEdit::setToolTip);
    connect(ui->leExpF, &QLineEdit::textChanged, this, &Exporter::toggleExportButton);
    connect(ui->leExpF, &QLineEdit::textChanged, this, &Exporter::updateExportFolder);
    connect(ui->leExpF, &QLineEdit::editingFinished, glb, &QSettings::sync);
    connect(this, &Exporter::exportFolderChanged, this, [=](const QString &pth){fsmodel->setRootPath(QDir(pth).rootPath());});
    connect(this, &Exporter::exportFolderChanged, this, &Exporter::toggleExportButton);

    // Browse Button
    connect(ui->tbBrowse, &QToolButton::clicked, this, [=](){pickExportFolder(ui->leExpF->text());});
    connect(ui->pbExport, &QPushButton::clicked, this, &Exporter::requestExport);
    connect(ui->leQty, &QLineEdit::textChanged, this, &Exporter::toggleExportButton);
    connect(ui->leBahan, &QLineEdit::textChanged, this, &Exporter::toggleExportButton);
    connect(ui->txKet, &QPlainTextEdit::textChanged, this, &Exporter::toggleExportButton);

    connect(this, &Exporter::saveData, this, &Exporter::on_saveData);

    ui->pBar->setHidden(true);

    // Database Table
    db->setParent(this);
    ui->histTable->verticalHeader()->setMinimumSectionSize(15);
    ui->histTable->verticalHeader()->setDefaultSectionSize(18);
    // ui->histTable->verticalHeader()->setHidden(true);
    ui->histTable->horizontalHeader()->setMinimumHeight(15);
    ui->histTable->horizontalHeader()->setMaximumHeight(18);

    // QSortFilterProxyModel *prox = new QSortFilterProxyModel(this);
    A3PDataModel *src = db->a3dataTable();
    src->setTable("a3pdata");
    src->setObjectName("source_model");
//    qDebug() << A3PDataModel::m_instance_count;
//    if(src->lastError().isValid())
//        qDebug() << src->lastError();
    // prox->setSourceModel(src);
    // prox->setFilterKeyColumn(-1);
    // prox->setRecursiveFilteringEnabled(true);
//    prox->setObjectName("model_proxy");
//    prox->setFilterCaseSensitivity(Qt::CaseInsensitive);
//    prox->setDynamicSortFilter(false);
    // prox->moveColumn(src->index(0, 0), 2, prox->index(0, 0), 0);
    ui->histTable->setModel(src);
    src->setMaxRow(200);
    src->setFilter();
    ui->histTable->hideColumn(0);
    ui->histTable->hideColumn(1);
    ui->histTable->sortByColumn(0, Qt::AscendingOrder);
    ui->histTable->model()->setHeaderData(2, Qt::Horizontal, "Klien");
    ui->histTable->model()->setHeaderData(3, Qt::Horizontal, "File");
    ui->histTable->model()->setHeaderData(4, Qt::Horizontal, "Bahan");
    ui->histTable->model()->setHeaderData(5, Qt::Horizontal, "Jumlah");
    ui->histTable->model()->setHeaderData(6, Qt::Horizontal, "Duplikat");
    ui->histTable->model()->setHeaderData(7, Qt::Horizontal, "Sisi");
    ui->histTable->model()->setHeaderData(8, Qt::Horizontal, "Keterangan");
    ui->histTable->model()->setHeaderData(9, Qt::Horizontal, "NamaFile");
    ui->histTable->hideColumn(10);
    ui->histTable->setSortingEnabled(true);
    ui->histTable->setSelectionBehavior(ui->histTable->SelectRows);
    ui->histTable->setAlternatingRowColors(true);
    ui->histTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(src, &QAbstractItemModel::rowsRemoved, src, &A3PDataModel::submit);
    connect(ui->histTable, &QTableView::customContextMenuRequested, this, &Exporter::tableContextMenu);


    // Completer Bahan
    QCompleter *bahanCompleter = new QCompleter(ui->leBahan);
    QSqlQueryModel *bahanModel = new QSqlQueryModel(ui->leBahan);
    bahanModel->setQuery("SELECT DISTINCT bahan FROM a3pdata ORDER BY bahan ASC");
    bahanCompleter->setModel(bahanModel);
    bahanCompleter->setCompletionColumn(0);
    bahanCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->leBahan->setCompleter(bahanCompleter);
    connect(db, &A3DataBase::dataUpdated, bahanModel,
            [=](){bahanModel->setQuery(bahanModel->query().lastQuery());});

    // Completer Klien
    QCompleter *klienCompleter = new QCompleter(ui->leKlien);
    QSqlQueryModel *klienModel = new QSqlQueryModel(ui->leKlien);
    klienModel->setQuery("SELECT DISTINCT klien FROM a3pdata ORDER BY klien ASC");
    klienCompleter->setModel(klienModel);
    klienCompleter->setCompletionColumn(0);
    klienCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->leKlien->setCompleter(klienCompleter);
    connect(db, &A3DataBase::dataUpdated, klienCompleter,
            [=](){klienModel->setQuery(klienModel->query().lastQuery());});

    // ViewRange
    connect(ui->cbxViewRange,&QComboBox::currentTextChanged, src, &A3PDataModel::toggleShowMode);
    connect(ui->cbxViewRange,&QComboBox::currentTextChanged, ui->leFilter, &QLineEdit::clear);
    connect(ui->cbxViewRange,&QComboBox::currentTextChanged, ui->histTable,
            [=]()
                    {
                        ui->histTable->sortByColumn(0, Qt::AscendingOrder);
                    });
    // Navigasi
    connect(src, &A3PDataModel::currentPageChanged, this, &Exporter::manageNavigasi);
    connect(src, &A3PDataModel::filterChanged, this, &Exporter::manageNavigasi);
    connect(ui->btNext, &QToolButton::clicked, src, &A3PDataModel::nextPage);
    connect(ui->btPrev, &QToolButton::clicked, src, &A3PDataModel::prevPage);
    connect(ui->btLast, &QToolButton::clicked, src, &A3PDataModel::lastPage);
    connect(ui->btFirst, &QToolButton::clicked, src, &A3PDataModel::firstPage);


    // ShortCUT
    QShortcut *scExport = new QShortcut(this);
    QShortcut *scClDoc = new QShortcut(this);
    QShortcut *scCTgHist = new QShortcut(this);
    scExport->setKey(QKeySequence("Ctrl+e")); // Export
    scClDoc->setKey(QKeySequence("Ctrl+q"));  // Close
    scCTgHist->setKey(QKeySequence("Ctrl+h"));  // Close

    connect(scCTgHist, &QShortcut::activated, this, [=](){
        ui->histGroup->setVisible(!ui->histGroup->isVisible());
        adjustSize();});
    connect(scExport, &QShortcut::activated, this, [=](){if(ui->pbExport->isEnabled()) ui->pbExport->click();});
    connect(scClDoc, &QShortcut::activated, this, [=](){
            QVariantMap task;
            task["corelVersion"] = ui->comboVersi->currentData(Qt::UserRole + 1);
            // corel.setTask(task);
            CorelManager *man = SingletonNS::manager;
            emit man->cldoc(task);
    });
    

    // setFixedSize(this->size());
    manageNavigasi();

}

Exporter::~Exporter()
{
    qsm->detach();
    qsm->deleteLater();
    db->deleteLater();
    
    delete ui;
    delete glb;
}

void Exporter::handleCorelResult(const QMap<QString, QVariant> &res)
{   
    if(!res.value("getCorel").toBool())
    {
        ui->corelGroup->setDisabled(true);
        ui->corelGroup->setToolTip("Corel Error");
        QList<QWidget*> group = ui->corelGroup->findChildren<QWidget*>();
        for(auto w = group.begin(); w!=group.end(); ++w)
            (*w)->setToolTip("Corel Error");
        ui->btPdfSetting->setDisabled(true);
        ui->leKlien->clear();
        ui->leFile->clear();
        ui->leBahan->clear();
        ui->leQty->clear();
        ui->sideCheck->setChecked(false);
        ui->txKet->clear();
        ui->pbExport->setDisabled(true);
        return ;
    } else {
        ui->corelGroup->setEnabled(true);
        ui->corelGroup->setToolTip("");
        QList<QWidget*> group = ui->corelGroup->findChildren<QWidget*>();
        for(auto w = group.begin(); w!=group.end(); ++w)
            (*w)->setToolTip("");
    }

    int corelVersion =  res.value("corelVersion").toInt();
    int versionIndex = ui->comboVersi->findText(res.value("corelVersion").toString());
    ui->comboVersi->setItemText(versionIndex, corelVersion ? QString("%1").arg(corelVersion) : "Auto");
    glb->setValue("CorelApplication/useVersion", corelVersion);
    glb->sync();
    if(!res.value("getDocuments").toBool())
    {
        ui->leDoc->setText("Tidak ada file");
        ui->leDoc->setToolTip("");
        ui->leKlien->clear();
        ui->leFile->clear();
        ui->leBahan->clear();
        ui->leQty->clear();
        ui->lePage->clear();
        ui->sideCheck->setChecked(false);
        ui->txKet->clear();
        ui->pbExport->setDisabled(true);

    }
    else
    {
        if(!res["docSaved"].toBool())
        {
            ui->leDoc->setText(res["name"].toString());
            ui->leDoc->setToolTip("File ini belum di simpan");
            ui->leKlien->clear();
            ui->lePage->setText(res.value("pageCount").toInt() > 1 ? QString("1-%1").arg(res["pageCount"].toInt()) : "1");
            ui->leFile->setText(res["name"].toString());
            ui->leBahan->clear();
            ui->sideCheck->setChecked(false);
            ui->txKet->clear();
            ui->pbExport->setDisabled(true);
            ui->btPdfSetting->setDisabled(false);
        }
        else
        {
            QString fPath = QDir::fromNativeSeparators(res.value("filePath").toString() + res.value("fileName").toString());
            ui->leDoc->setText(res["fileName"].toString());
            ui->leDoc->setToolTip(fPath);
            ui->leKlien->setText(QDir(res.value("filePath").toString()).dirName().toUpper());
            ui->leKlien->setToolTip(QDir(res["filePath"].toString()).absolutePath());
            ui->leFile->setText(QFileInfo(fPath).completeBaseName().replace("_", " ").simplified().toUpper());
            ui->lePage->setText(res.value("pageCount").toInt() > 1 ? QString("1-%1").arg(res["pageCount"].toInt()) : "1");
            ui->leBahan->clear();
            ui->sideCheck->setChecked(false);
            ui->txKet->clear();
            ui->pbExport->setDisabled(true);
            ui->btPdfSetting->setDisabled(false);
        }
    }
    toggleExportButton();
}

void Exporter::handleExportResult(const QMap<QString, QVariant> &res)
{
    QString finalExportPath;
    ui->pBar->setMaximum(0);
    ui->pBar->setValue(0);
    if(res.contains("corelMessage") && res["corelMessage"].toString() == "exportBegin")
        ui->pBar->setVisible(true);
    if(res.contains("corelMessage") && res["corelMessage"].toString() == "exportFinish")
        ui->pBar->setVisible(false);
    if(res.contains("pdfsaved") && res["pdfsaved"].toBool())
    {
        ui->pBar->setVisible(false); // what to do ?
        ui->pBar->setMaximum(100);
        ui->pBar->setValue(100);
        QLabel *st = new QLabel("Memindahkan File ...", ui->paramGroup);
        st->setGeometry(ui->pBar->geometry());
        st->setAlignment(Qt::AlignCenter);
        st->show();
        connect(ui->pBar, &QProgressBar::valueChanged, st, &QLabel::deleteLater);
    }
    if(res.contains("pdfmoved"))
    {
        if(!res["pdfmoved"].toBool())
        {
            QFileInfo exportAs(res["exportName"].toString());
            QMessageBox::warning(this, "File gagal di simpan", res["errMsg"].toString());
            QString newFileName = QFileDialog::getSaveFileName(this, "Simpan sebagai", exportAs.absoluteFilePath(), "PDF Files (*pdf)");
            if(!newFileName.length())
            {
                QFile::remove(res["fileName"].toString());
            }
            else
            {
                finalExportPath = QFileInfo(newFileName).filePath();
                QFile tmp(res["fileName"].toString());
                tmp.rename(newFileName);
                emit saveData(finalExportPath);
            }
        }
        else
        {

            finalExportPath = QFileInfo(res["exportName"].toString()).filePath();
            emit saveData(finalExportPath);
            QWidget *box = new QWidget();
            box->setLayout(new QHBoxLayout(box));
            QGroupBox *gb = new QGroupBox(box);
            gb->setLayout(new QHBoxLayout(gb));
            gb->setTitle("Pemberitahuan");
            box->setWindowFlag(Qt::FramelessWindowHint);
            box->setContentsMargins( 5, 5, 5, 5);
            QLabel *lmessage = new QLabel(QString("Berhasil mengexport :\n%1").arg(finalExportPath), gb);
            lmessage->setWordWrap(true);
            QFont f = lmessage->font();
            f.setPointSizeF(f.pointSizeF() * 1.05);
            lmessage->setFont(f);
            gb->layout()->addWidget(lmessage);
            box->layout()->addWidget(gb);
            box->adjustSize();
            QSize pgSz = QGuiApplication::primaryScreen()->size();
            int x, y;
            x = (pgSz.width() - box->sizeHint().width()) / 2;
            y = (pgSz.height() - box->sizeHint().height()) / 2;
            box->move(x, y);
            // lmessage->setStyleSheet("color: rgb(0, 0, 200); background-color: rgb(255, 255, 180); border-radius:15px");
            box->setAttribute(Qt::WA_ShowWithoutActivating);
            box->setAttribute(Qt::WA_AlwaysStackOnTop);
            // box->setBackgroundRole(QPalette::Dark);
            // lmessage->setForegroundRole(QPalette::Dark);
            box->show();
            QTimer::singleShot(2000, box, SLOT(deleteLater()));
//            QMessageBox::information(this, "Selesai", QString("Export :\n\"%1\" berhasil")
//                                     .arg(QFileInfo(res["exportName"].toString()).fileName()));
        }
        ui->pBar->setMaximum(0);
        ui->pBar->setValue(0);
        ui->pBar->hide();
    }
}

QString Exporter::currentExportFolder() const
{
    return ui->leExpF->text();
}

void Exporter::on_tbDet_clicked()
{
    auto *mgr = SingletonNS::manager;
    emit mgr->detect(QVariantMap {{"corelVersion", ui->comboVersi->currentData(Qt::UserRole + 1)}});
    toggleExportButton();
}

void Exporter::updateQty()
{
    int kalk = KalkulasiJumlahHalaman(ui->lePage->text());
    if(kalk > 0)
    { 
        if(!(kalk % 2))
        {
            ui->sideCheck->setEnabled(true);
            if(ui->sideCheck->isChecked())
                if(kalk / 2 > 1)
                    ui->leQty->setText(QString("%1@1").arg(kalk / 2));
                else
                    ui->leQty->setText(QString("%1").arg(kalk / 2));

            else
                ui->leQty->setText(QString("%1@1").arg(kalk));
        }
        else
        {
            ui->sideCheck->setEnabled(false);
            ui->sideCheck->setChecked(false);
            if(kalk == 1)
                ui->leQty->setText("1");
            else
                ui->leQty->setText(QString("%1@1").arg(kalk));
        }
    }
    QString
    toggleExportButton();
}

QString Exporter::pickExportFolder(const QString &current=QString())
{
    QString fromDir;
    if(current.length()) {
        fromDir = current;
    }
    else
        fromDir = QDir::currentPath();

    QString expDir = QFileDialog::getExistingDirectory(nullptr, "Pilih direktori export", fromDir);
    if(expDir.length())
        emit exportFolderChanged(QDir::toNativeSeparators(expDir));
    return expDir;
}

void Exporter::requestExport()
{
    QVariantMap m;
    m.insert("taskType", "export");
    QString targetFileName = QString("%1_%2_%3_%4")
            .arg(ui->leKlien->text().simplified().remove("_").toUpper(),
                 ui->leFile->text().simplified().remove("_").toUpper(),
                 ui->leBahan->text().simplified().remove("_").toUpper(),
                 ui->leQty->text().simplified().remove("_").toUpper());
    if(ui->sideCheck->isChecked())
        targetFileName += "_BB";
    if(ui->txKet->toPlainText().simplified().toUpper().length())
        targetFileName += QString("_%1").arg(ui->txKet->toPlainText().simplified().toUpper());
    targetFileName += ".pdf";
    m.insert("autoCurves", ui->kurvaOto->isChecked());
    m.insert("pageRange", ui->lePage->text());
    m.insert("fileName", targetFileName);
    m.insert("dirName", ui->leExpF->text());
    auto *mgr = SingletonNS::manager;
    m["corelVersion"] = ui->comboVersi->currentData(Qt::UserRole + 1);
    emit mgr->exp(m);
}

void Exporter::toggleExportButton()
{
    exportName = "";
    if(ui->pbExport->isEnabled())
        ui->pbExport->setDisabled(true);
    if(ui->leExpF->text().isEmpty())
        return;
    else {
        auto fexist = QFileInfo::exists(ui->leExpF->text());
        if(!fexist)
            return;
        if(!QFileInfo(ui->leExpF->text()).isDir())
            return;
    }
    exportName += ui->leExpF->text();
    exportName += "/";

    if(ui->leKlien->text().isEmpty())
        return;
    exportName += ui->leKlien->text();

    if(ui->leFile->text().isEmpty())
        return;
    exportName += "_";
    exportName += ui->leFile->text();

    if(ui->leBahan->text().isEmpty())
        return ;
    exportName += "_";
    exportName += ui->leBahan->text();

    if(ui->leQty->text().isEmpty())
        return;
    exportName += "_";
    exportName += ui->leQty->text();

    if(ui->sideCheck->isChecked())
    {
        exportName += "_BB";
    }

    QString ket = ui->txKet->toPlainText().simplified();
    if(ket.length())
        exportName += QString("_%1").arg(ket);
    ui->pbExport->setEnabled(true);
}

void Exporter::disablesAll()
{
    ui->tbDet->setDisabled(true);
    ui->pbExport->setDisabled(true);
}

void Exporter::enablesAll()
{
    ui->tbDet->setDisabled(false);
}

void Exporter::tableContextMenu(const QPoint &pos)
{
    if(!ui->histTable->selectionModel()->hasSelection())
        return ;
    auto selectedRow = ui->histTable->selectionModel()->selectedRows();
    auto model = ui->histTable->model();
    QString fdata = selectedRow.at(0).siblingAtColumn(9).data(Qt::DisplayRole).toString();
    QMenu tconMenu("Pilihan", ui->histTable);
    QAction *delRow = new QAction(tr("&Hapus"));
    QAction *edt = new QAction(tr("&Edit"));
    connect(delRow, &QAction::triggered, ui->histTable,
            [=]()
                {
                    int result = QMessageBox::question(this, "Konfirmasi",
                                          QString("Hapus data\n%1?").arg(fdata));
                    if(result == QMessageBox::Yes)
                    {
                        bool ok = model->removeRow(selectedRow[0].row());
                        if(ok)
                            QMessageBox::information(this, "Info", "Berhasil di hapus :\n" + fdata);
                    }
                    else
                        QMessageBox::information(this, "Info", "Dibatalkan");
                }
    );
    tconMenu.addAction(delRow);
    tconMenu.addAction(edt);

    QPoint gp = ui->histTable->mapToGlobal(pos);
    tconMenu.exec(gp);
}

QVariant CustomClassNS::FileSystemModel::data(const QModelIndex &index, int role) const
{
    if(((role == Qt::EditRole) & (index.column() == 0)))
    {
        QString pth = QDir::fromNativeSeparators(filePath(index));
        if(pth.endsWith("/"))
            pth.chop(1);
        return pth;
    }
    return QFileSystemModel::data(index, role);
}

QString CustomClassNS::FSCompleter::pathFromIndex(const QModelIndex &idx) const
{
    QString rv = model()->data(idx, Qt::EditRole).toString();
    return rv;
}

void Exporter::on_btPdfSetting_clicked()
{
    QVariantMap m;
    m.insert("taskType", "openSettings");
    m["corelVersion"] = ui->comboVersi->currentData(Qt::UserRole + 1);
    auto *mgr = SingletonNS::manager;
    emit mgr->sett(m);
}

void Exporter::on_saveData(const QString &p)
{
    QString fname = QFileInfo(p).fileName();
    db->insert(fname);
}

void Exporter::manageNavigasi()
{
    A3PDataModel *model = db->a3dataTable();
    assert(model);
    ui->btNext->setEnabled(model->hasNextPage());
    ui->btPrev->setEnabled(model->hasPrevPage());
    ui->btFirst->setEnabled(model->hasPrevPage());
    ui->btLast->setEnabled(model->hasNextPage());
    ui->spHalaman->setValue(model->currentPage() + 1);
    ui->spHalaman->setSuffix(QString("/%1").arg(model->maxPage()));
}

void Exporter::on_pbFilter_clicked()
{
    A3PDataModel *mod = findChild<A3PDataModel*>("source_model");
    assert(mod);
    QString flt =
            " klien || \" \" ||"
            " file || \" \" ||"
            " bahan || \" \" ||"
            " jkertas || \" \" ||"
            " jkopi || \" \" ||"
            " sisi || \" \" ||"
            " (SELECT CASE WHEN keterangan IS NULL THEN \"\" ELSE keterangan END ) || \" \" || "
            "(jkertas * jkopi * sisi) LIKE \"%%1%\"",
            ftext = ui->leFilter->text().simplified();
    if(!ftext.isEmpty())
        flt = flt.arg(ftext);
    else
        flt = "";
    mod->setFilter(flt);
    // qDebug() << mod->filter();
}

void Exporter::on_pbDetach_clicked()
{
    A3PreviewDataDialog *prvDlg = findChild<A3PreviewDataDialog*>("prvDialog");
    if(!prvDlg)
            prvDlg = new A3PreviewDataDialog(this);
    prvDlg->show();
    connect(prvDlg, &A3PreviewDataDialog::tableUpdated, this, [&](){
        A3PDataModel *mod = findChild<A3PDataModel*>("source_model");
        mod->setTable(mod->tableName());
    });
    connect(prvDlg, &A3PreviewDataDialog::destroyed, prvDlg, &A3PreviewDataDialog::deleteLater);
}

void Exporter::updateExportFolder(const QString &name)
{
    glb->setValue("Exporter/lastExportFolder", name);
}

void Exporter::on_pushButton_clicked()
{
    TentangAplikasi *ta = new TentangAplikasi();
    connect(ta, SIGNAL(destroyed()), ta, SLOT(deleteLater()));
    ta->show();
}

void Exporter::comboVersiChanged(int index)
{
    QString message;
    QString old = glb->value("CorelApplication/useVersion").toString(),
            _new = ui->comboVersi->itemText(index);
            message = "Pastikan anda telah menginstall Corel Versi %1 "
                      "Lanjutkan perubahan Corel atau tetap menggunakan Versi %2";
    
    int result = QMessageBox::information(this, "Konfirmasi Corel yang digunakan",
                                          message.arg(_new, old), QMessageBox::Ok, QMessageBox::No);
    if(result == QMessageBox::No) {
        ui->comboVersi->setCurrentText(old);
        QMessageBox::information(this, "Info", "Dibatalkan");
        return;
    }
    glb->setValue("CorelApplication/useVersion", _new);
    glb->sync();
}

