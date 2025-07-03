#include "incl/exporter.h"
#include "ui/ui_exporter.h"
#include "incl/a3previewdatadialog.h"
#include "incl/tentangaplikasi.h"
#include "incl/a3database.h"
#include "corelmanager/incl/corelexecutor.h"
#include "incl/models.h"
#include "incl/filemover.h"

#include <QCompleter>
#include <QRegularExpression>
#include <QAbstractItemModel>
#include <QRegularExpressionValidator>
#include <QStringListModel>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QSharedMemory>

#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMenu>
#include <QShortcut>

#include <QMessageBox>
#include <QCoreApplication>
#include <QtDebug>
#include <QScreen>

Exporter::Exporter(QWidget *parent)
    : ui(new Ui::Exporter),
      executorThread(new QThread),
      cx(new CorelExecutor),
      QWidget(parent)
{
    A3PDataModel* model = new A3PDataModel(this);
    model->setObjectName("A3PDataModel");
    ui->setupUi(this);
    ui->pBar->hide();
    QSettings* glb = new QSettings("conf.ini", QSettings::IniFormat, this);
    glb->setObjectName("settings");
    ui->comboVersi->clear();
    addAction(ui->actionToggleHistory);
    addAction(ui->actionRefreshHistory);
    addAction(ui->actionCloseDocument);
    QSettings corelLookup("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
    QString corelWithVersion = "CorelDRAW.Application.%1";
    
    for(int i=14; i<100; ++i) {
        QString vstr = QString::number(i);
        QString ProgID = corelWithVersion.arg(i);
        QString CLSIDKey = QString("%1/CLSID/.").arg(ProgID);
        QString CLSID = corelLookup.value(CLSIDKey).toString();
        if(!CLSID.isEmpty()) {
            ui->comboVersi->addItem(vstr, ProgID);
            int lastIndex = ui->comboVersi->findText(vstr);
            ui->comboVersi->setItemData(lastIndex, CLSID, CLSIDRole);
        }
    }
    if(ui->comboVersi->count() > 1) {
        ui->comboVersi->setCurrentIndex(ui->comboVersi->count() - 1);
    }
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
    
    model->setMaxRow(200);
    model->setFilter();
    model->select();
    ui->histTable->setModel(model);
    ui->histTable->hideColumn(0);
    ui->histTable->hideColumn(1);
    ui->histTable->sortByColumn(0, Qt::DescendingOrder);
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
    connect(model, &QAbstractItemModel::rowsRemoved, model, &A3PDataModel::submit);
    connect(ui->cbxViewRange,&QComboBox::currentTextChanged, model, &A3PDataModel::toggleShowMode);

    connect(ui->btNext, &QToolButton::clicked, model, &A3PDataModel::nextPage);
    connect(ui->btPrev, &QToolButton::clicked, model, &A3PDataModel::prevPage);
    connect(ui->btLast, &QToolButton::clicked, model, &A3PDataModel::lastPage);
    connect(ui->btFirst, &QToolButton::clicked, model, &A3PDataModel::firstPage);
    
    ui->sideCheck->setEnabled(false);
    ui->sideCheck->setChecked(false);
    
    
    auto bahanCompleter = new QCompleter(this);
    auto bahanModel = new QSqlQueryModel(this);
    bahanCompleter->setObjectName("bahanCompleter");
    bahanCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    bahanModel->setObjectName("bahanModel");
    bahanModel->setQuery("SELECT bahan, SUM(jkopi) as [count] FROM a3pdata GROUP BY bahan ORDER BY [count] DESC");
    bahanCompleter->setModel(bahanModel);
    bahanCompleter->setCompletionMode(QCompleter::PopupCompletion);
    ui->leBahan->setCompleter(bahanCompleter);
    
    auto editorModel = new QStringListModel(QStringList {"A3P", "BRY", "GAL", "PLH"}, this);
    editorModel->setObjectName("editorModel");
    ui->comboBox->setModel(editorModel);
    
    auto klienCompleter = new QCompleter(this);
    klienCompleter->setObjectName("klienCompleter");
    klienCompleter->setCompletionMode(QCompleter::PopupCompletion);
    klienCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    auto klienModel = new QSqlQueryModel(this);
    klienModel->setObjectName("klienModel");
    klienModel->setQuery("SELECT DISTINCT klien FROM a3pdata");
    klienCompleter->setModel(klienModel);
    ui->leKlien->setCompleter(klienCompleter);
    
    cx->moveToThread(executorThread);
    
    connect(executorThread, &QThread::started, cx, &CorelExecutor::init);
    connect(this, &Exporter::requestDetect, cx, &CorelExecutor::runDetect);
    connect(cx, &CorelExecutor::detectResult, this, &Exporter::detectResultReady);
    connect(this, &Exporter::requestOpenSettings, cx, &CorelExecutor::openSettings);
    connect(cx, &CorelExecutor::pdfSettingsChanged, this, &Exporter::pdfSettingsChanged);
    connect(cx, &CorelExecutor::pdfSettingsResult, this, &Exporter::pdfSettingsResult);
    connect(this, &Exporter::requestExport, cx, &CorelExecutor::runExport);
    connect(cx, &CorelExecutor::exportResult, this, &Exporter::exportResultReady);
    connect(this, &Exporter::kurvaOtoChanged, cx, &CorelExecutor::enableAutoCurve);
    connect(this, &Exporter::requestClose, cx, &CorelExecutor::closeActiveDocument);
    connect(cx, &CorelExecutor::activeDocumentClosed, this, &Exporter::documentClosedHandler);
    // connect(cx, &CorelExecutor::moveFailed, this, &Exporter::onMoverFailed);
    connect(this, &Exporter::readyToMove, this, &Exporter::moveExportedFile);
    executorThread->start();
    
    connect(this, &Exporter::exported, bahanModel, [bahanModel](){ bahanModel->setQuery(bahanModel->query().lastQuery()); });
    connect(this, &Exporter::exported, klienModel, [klienModel](){ klienModel->setQuery(klienModel->query().lastQuery()); });
    
#ifdef DEVEL_MODE_ON
    setWindowTitle(windowTitle() + " - Development");
#endif
}

Exporter::~Exporter()
{
    QSettings* glb = findChild<QSettings*>("settings");
    glb->sync();
    delete ui;
}

void Exporter::on_leExpF_textChanged(const QString& tx) {
    QSettings* glb = findChild<QSettings*>("settings");
    glb->setValue("Exporter/lastExportFolder", tx);
}

void Exporter::on_actionCloseDocument_triggered()
{
    auto CLSID = ui->comboVersi->currentData(CLSIDRole).toString();
    emit requestClose(CLSID);
}

void Exporter::detectResultReady(const QVariantMap& res)
{
    waitState[QString("%1_detect").arg(res["controlName"].toString())] = false;
    ui->tbDet->setEnabled(true);
    if(!res["state"].toBool()) {
        QMessageBox::information(this, "Kesalahan", QString("Error :\n%1").arg(res["stateMessage"].toString()));
        return ;
    }
    ui->leDoc->clear();
    ui->leBahan->clear();
    ui->leQty->clear();
    ui->leKlien->clear();
    ui->leFile->clear();
    ui->lePage->clear();
    ui->txKet->clear();
    if(res["documentCount"].toInt() < 1) {
        ui->leDoc->setText("Tidak ada file");
    } else {
        ui->leDoc->setText(res["fileName"].toString().isEmpty() ? res["vbaName"].toString() : res["fileName"].toString());
        if(!res["filePath"].toString().isEmpty()) {
            ui->leKlien->setText(QDir(res["filePath"].toString()).dirName().toUpper());
        }
        ui->lePage->setText(res["pageCount"].toInt() > 1 ? QString("1-%1").arg(res["pageCount"].toInt()) : "1");
        QString baseName = QFileInfo(res["vbaName"].toString()).completeBaseName();
        ui->leFile->setText(baseName.replace("_", "-").toUpper());
    }
}

void Exporter::pdfSettingsResult(const QVariantMap& res) {
    waitState[QString("%1_openSettings").arg(res["controlName"].toString())] = false;
    ui->btPdfSetting->setEnabled(true);
    if(!res["state"].toBool()) {
        QMessageBox::information(this, "Kesalahan", QString("Error :\n%1").arg(res["stateMessage"].toString()));
        return ;
    }
}

void Exporter::moveExportedFile(const QVariantMap& res)
{
    QDir exportPath(res["exportPath"].toString());
    QFile exported(res["tempFile"].toString());
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");
  
    if(exportPath.exists(res["exportName"].toString())) {
        // Pertimbangkan untuk membuat loop dialog sampai proses pemindahan dan atau penghapusan file temporer berhasil
        QMessageBox::critical(this, "Duplikasi file ditemukan", "Anda dapat membatalkan atau menyimpan dengan nama file berbeda pada dialog selanjutnya");
        QString saveAs = QFileDialog::getSaveFileName(this, "Simpan sebagai", exportPath.absoluteFilePath(res["exportName"].toString()), ("PDF File (*.pdf)"));
        if(saveAs.isEmpty()) {
            if(exported.remove()) {
                QMessageBox::information(this, "Informasi", "Export dibatalkan");
                ui->pBar->hide();
                return ;
            } else {
                QMessageBox::information(this, "Informasi", "Gagal menghapus berkas sementara, anda harus menghapusnya secara manual");
                QProcess::startDetached("C:\\Windows\\explorer.exe", QStringList() << QFileInfo(res["tempFile"].toString()).absolutePath());
                ui->pBar->hide();
                return;
            }
        } else {
            QFileInfo sa(saveAs);
            if(sa.exists()) {
                QFile saf(saveAs);
                bool rmsuc = saf.remove();
                if(!rmsuc) {
                    QMessageBox::information(this, "Informasi", "Gagal menghapus berkas target");
                    ui->pBar->hide();
                    return;
                }
            }
            bool eok = exported.rename(saveAs);
            if(!eok)
                QMessageBox::information(this, "Informasi", "Gagal menyimpan file export");
            ui->pBar->hide();
            return ;
        }
    }
    
    statusLabel->setText("Memindahkan file...");
    
    bool eok = exported.rename(exportPath.absoluteFilePath(res["exportName"].toString()));
    
    QFileInfo fi(exported);
    if(!eok) {
        QMessageBox::information(this, "Informasi", "Gagal menyimpan file export");
    } else {
        statusLabel->setText("Selesai...");
        emit this->exported(fi.fileName());
        QTimer::singleShot(0, qobject_cast<A3PDataModel*>(ui->histTable->model()), &A3PDataModel::select);
    }
    QMessageBox* mb = new QMessageBox(QMessageBox::Information, "Export selesai", fi.fileName(), QMessageBox::NoButton);
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->setWindowFlag(Qt::FramelessWindowHint, true);
    QTimer::singleShot(2000, mb, &QDialog::accept);
    mb->open();
    ui->pBar->hide();
}

void Exporter::handleMoveStart() {
    QLabel *lab = findChild<QLabel*>("statusLabel");
    lab->setText("Memindahkan file");
}

void Exporter::handleMoveDone(const QVariantMap& par) {
    if(par["status"].toBool()) {
        QLabel *lab = findChild<QLabel*>("statusLabel");
        lab->setText("Selesai memindahkan");
        emit exported(par["target"].toString());
        auto model = findChild<A3PDataModel*>("A3PDataModel");
        model->select();
        ui->pBar->hide();
        return;
    }
    QMessageBox::information(this, "Gagal memindahkan File", QString("Temp : %1\nKe : %2").arg(par["tempFile"].toString()).arg(par["target"].toString()));
    ui->pBar->hide();
}

void Exporter::exportResultReady(const QVariantMap& res)
{

    waitState[QString("%1_export").arg(res["controlName"].toString())] = false;
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");
    ui->pbExport->setEnabled(true);
    if(!res["state"].toBool()) {
        QMessageBox::information(this, "Kesalahan", QString("Error :\n%1").arg(res["stateMessage"].toString()));
        ui->pBar->hide();
        return ;
    }
    
    QFile exported(res["tempFile"].toString());
    if(!exported.exists()) {
        QMessageBox::critical(this, "Kesalahan", "File export tidak ditemukan");
        ui->pBar->hide();
        return;
    }
    
    QDir exportPath(res["exportPath"].toString());
    QString exportName = exportPath.absoluteFilePath(res["exportName"].toString());
    if( exportPath.exists(res["exportName"].toString()) ) {
        // get new name
        QString newName = QFileDialog::getSaveFileName(this, "Simpan dengan Nama", exportPath.absoluteFilePath(res["exportName"].toString()), "PDF (*.pdf)");
        if( newName.isEmpty() ) {
            QFile(res["tempFile"].toString()).remove();
            QMessageBox::information(this, "Informasi", "Export dibatalkan");
            return ;
        }
        if(QFileInfo::exists(newName)) {
            QFile q(newName);
            q.remove();
        }
        exportName = newName;
    }
    
    QVariantMap vmap;
    vmap.insert("tempFile", res["tempFile"]);
    vmap.insert("target", exportName);
    
    FileMover* fmov = new FileMover(vmap);
    connect(fmov, &QThread::started, this, &Exporter::handleMoveStart);
    connect(fmov, &FileMover::moverDone, this, &Exporter::handleMoveDone);
    connect(fmov, &QThread::finished, fmov, &QThread::deleteLater);
    fmov->start();
}

void Exporter::pdfSettingsChanged(const QVariantMap& m){
    QMessageBox msgBox(QMessageBox::Question, "Konfirmasi Perubahan", "Perubahan parameter PDF terdeteksi terapkan perubahan?");
    msgBox.addButton("Hanya Sekali", QMessageBox::RejectRole);
    msgBox.addButton("Selamanya", QMessageBox::AcceptRole);
    if( msgBox.exec() == QMessageBox::Accepted ) {
        QSettings* glb = findChild<QSettings*>("settings");
        glb->setValue("PDFSettings", m);
        glb->sync();
    }
    ui->btPdfSetting->setDisabled(false);
}

void Exporter::documentClosedHandler()
{
    if(ui->tbDet->isEnabled())
        ui->tbDet->click();
}

QString Exporter::currentExportFolder() const
{
    return ui->leExpF->text();
}

void Exporter::on_tbDet_clicked()
{
    QString controlName, progId;
    controlName = ui->comboVersi->currentData(CLSIDRole).toString();
    progId = ui->comboVersi->currentData(ProgIDRole).toString();
    if(waitState.value(QString("%1_detect").arg(controlName), false).toBool()) {
        QMessageBox::information(this, "Silahkan menunggu", QString("Perintah deteksi untuk %1 sebelumnya belum mendapat response").arg(progId));
        return;
    }
    waitState[QString("%1_detect").arg(controlName)] = true;
    ui->tbDet->setEnabled(false);
    emit requestDetect(controlName);
}

void Exporter::on_btPdfSetting_clicked()
{
    QString controlName, progId;
    controlName = ui->comboVersi->currentData(CLSIDRole).toString();
    progId = ui->comboVersi->currentData(ProgIDRole).toString();
    if(waitState.value(QString("%1_openSettings").arg(controlName), false).toBool()) {
        QMessageBox::information(this, "Silahkan menunggu", QString("Perintah OpenSettings untuk %1 sebelumnya belum mendapat response").arg(progId));
        return;
    }
    waitState[QString("%1_openSettings").arg(controlName)] = true;
    ui->btPdfSetting->setDisabled(true);
    emit requestOpenSettings(controlName);
}

void Exporter::on_tbBrowse_clicked()
{
    QString expDir = QFileDialog::getExistingDirectory(nullptr, "Pilih direktori export", ui->leExpF->text());
    if(!expDir.isEmpty()) {
        ui->leExpF->setText(QDir::toNativeSeparators(expDir));
        QSettings* glb = findChild<QSettings*>("settings");
        glb->setValue("Exporter/lastExportFolder", expDir);
    }
}

void Exporter::on_sideCheck_toggled(bool t) {
    int kalk = KalkulasiJumlahHalaman(ui->lePage->text());
    auto page = t ? kalk / 2 : kalk;
    if(page <= 1) {
        ui->leQty->setText(QString::number(page));
    } else {
        ui->leQty->setText(QString("%1@1").arg(page));
    }
}

void Exporter::on_pbExport_clicked()
{
    QString controlName, progId;
    controlName = ui->comboVersi->currentData(CLSIDRole).toString();
    progId = ui->comboVersi->currentData(ProgIDRole).toString();
    if(waitState.value(QString("%1_export").arg(controlName), false).toBool()) {
        QMessageBox::information(this, "Silahkan menunggu", QString("Perintah Export untuk %1 sebelumnya belum mendapat response").arg(progId));
        ui->pbExport->setEnabled(false);
        return;
    }
    
    QString klien, fname, bhn, pr, qtys;
    auto lmsg = [=](QWidget *which, const QString msg) {
        QMessageBox::information(this, "Kesalahan Input", msg);
        which->setFocus(Qt::OtherFocusReason);
    };
    if(ui->leKlien->text().isEmpty()) {
        lmsg(ui->leKlien, "Nama Konsumen tidak boleh kosong");
        return;
    }
    klien = ui->leKlien->text().replace("_", "-").toUpper();
    if(ui->leFile->text().isEmpty()) {
        lmsg(ui->leFile, "Nama File tidak boleh kosong");
        return;
    }
    fname = ui->leFile->text().replace("_", "-").toUpper();
    if(!ui->comboBox->currentText().isEmpty()) {
        fname = QString("%1 - %2").arg(ui->comboBox->currentText(), fname);
    }
    if(ui->leBahan->text().isEmpty()) {
        lmsg(ui->leBahan, "Nama Bahan tidak boleh kosong");
        return;
    }
    bhn = ui->leBahan->text().replace("_", "-").toUpper();
    if(ui->lePage->text().isEmpty()) {
        lmsg(ui->lePage, "PageRange tidak boleh kosong");
        return;
    }
    if(ui->leQty->text().isEmpty()) {
        lmsg(ui->leQty, "Qty tidak boleh kosong");
        return;
    }
    qtys = ui->leQty->text();
    if(ui->sideCheck->isEnabled() && ui->sideCheck->isChecked()) {
        qtys += "_BB";
    }
    if(!ui->txKet->toPlainText().isEmpty()) {
        qtys += QString("_%1").arg(ui->txKet->toPlainText().toUpper());
    }
    QStringList vars = {klien, fname, bhn, qtys};
    QVariantMap ep;
    ep["CLSID"] = controlName;
    ep["PageRange"] = ui->lePage->text();
    ep["exportPath"] = ui->leExpF->text();
    ep["exportName"] = vars.join("_") + ".pdf";

    waitState[QString("%1_export").arg(controlName)] = true;
    ui->pbExport->setDisabled(true);
    
    ui->pBar->setMinimum(0);
    ui->pBar->setMaximum(0);
    ui->pBar->show();
    
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");
    if(!statusLabel) {
        statusLabel = new QLabel(ui->pBar);
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setObjectName("statusLabel");
        statusLabel->setGeometry(ui->pBar->rect());
        statusLabel->show();
    }
    statusLabel->setText("Mengeksport...");
    emit requestExport(ep);
}

void Exporter::on_lePage_textChanged(const QString& txt)
{
    int kalk = KalkulasiJumlahHalaman(txt);
    auto lastCopy = parseQty(ui->leQty->text()).second;

    ui->sideCheck->blockSignals(true);
    auto isOdd = bool(kalk % 2);
    if(isOdd) {
        if(ui->sideCheck->isChecked())
            ui->sideCheck->setChecked(false);
        ui->sideCheck->setEnabled(false);
    } else {
        ui->sideCheck->setEnabled(true);
    }
    ui->sideCheck->blockSignals(false);
    
    if(kalk < 1)
    {
        ui->leQty->clear();
        return ;
    }
    
    int page = kalk;
    if(ui->sideCheck->isChecked()) {
        page = page / 2;
    }
    if(page > 1) {
        ui->leQty->setText(QString("%1@%2").arg(page).arg(lastCopy));
        return ;
    } else {
        ui->leQty->setText(QString("%1").arg(lastCopy));
        return ;
    }
}

void Exporter::on_leQty_textChanged(const QString& txt) {
    // just update tooltip
    auto kl = parseQty(txt);
    if (kl != QPair<int, int>{0, 0}) {
        QString tbl(R"(<style>table { border-spacing: 10px; } td { padding: 2px; }</style>
                    <table><tr><td align='right'><b>Lembar :</b></td><td align='right'>%1</td></tr>
                    <tr><td align='right'><b>Klik :</b></td><td align='right'>%2</td></tr></table>)");
        bool isBB = ui->sideCheck->isChecked();
        ui->leQty->setToolTip(tbl
                    .arg(locale().toString(kl.first))
                    .arg(locale().toString(kl.second * (isBB ? 2 : 1))));
        return ;
    }
    ui->leQty->setToolTip("");
}

void Exporter::on_histTable_customContextMenuRequested(const QPoint &pos)
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
    tconMenu.addAction(ui->actionRefreshHistory);
    tconMenu.addAction(delRow);
    tconMenu.addAction(edt);

    QPoint gp = ui->histTable->mapToGlobal(pos);
    tconMenu.exec(gp);
}

void Exporter::on_leFilter_textChanged(const QString& txt)
{
    A3PDataModel *mod = findChild<A3PDataModel*>("A3PDataModel");
    if(mod) {
		QString flt =
				" klien || \" \" ||"
				" file || \" \" ||"
				" bahan || \" \" ||"
				" jkertas || \" \" ||"
				" jkopi || \" \" ||"
				" sisi || \" \" ||"
				" COALESCE(keterangan, '') || \" \" || "
				"(jkertas * jkopi * sisi) LIKE \"%%1%\"",
				ftext = ui->leFilter->text().simplified();
		if(!ftext.isEmpty())
			flt = flt.arg(ftext);
		else
			flt = "";
		mod->setFilter(flt);
	}
  ui->histTable->sortByColumn(0, Qt::DescendingOrder);
  ui->histTable->resizeColumnsToContents();
}

void Exporter::on_pbDetach_clicked()
{
    A3PreviewDataDialog *prvDlg = new A3PreviewDataDialog();
    prvDlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(prvDlg, &A3PreviewDataDialog::tableUpdated, this, [&](){
        A3PDataModel *mod = findChild<A3PDataModel*>("source_model");
        mod->setTable(mod->tableName());
    });
    connect(prvDlg, &A3PreviewDataDialog::destroyed, [=](){ ui->pbDetach->setEnabled(true); });
    ui->pbDetach->setEnabled(false);
    prvDlg->show();
}

void Exporter::on_pushButton_clicked()
{
    TentangAplikasi *ta = new TentangAplikasi();
    connect(ta, SIGNAL(destroyed()), ta, SLOT(deleteLater()));
    ta->show();
}

void Exporter::on_comboVersi_currentIndexChanged(int index)
{}

void Exporter::on_actionToggleHistory_triggered()
{
    auto hidden = ui->histGroup->isHidden();
    ui->histGroup->setHidden(!hidden);
    setMaximumWidth(hidden ? 578 : 251);
    adjustSize();
}

void Exporter::on_actionRefreshHistory_triggered()
{
    if(!ui->histGroup->isHidden()) {
        ui->histTable->sortByColumn(0, Qt::DescendingOrder);
        ui->histTable->resizeColumnsToContents();
    }
}

void Exporter::on_A3PDataModel_filterChanged()
{
    A3PDataModel* md = qobject_cast<A3PDataModel*>(sender());
    if(!md) return;
    int minPage = 1, maxPage, curPage;
    maxPage = md->maxPage();
    curPage = md->currentPage();
    curPage += 1;
    ui->btFirst->setDisabled(curPage == 1);
    ui->btPrev->setDisabled(curPage >= 1);
    ui->btNext->setDisabled(curPage >= maxPage);
    ui->btLast->setDisabled(curPage >= maxPage);
    ui->spHalaman->setSuffix(QString("/%1").arg(maxPage));
    ui->spHalaman->setValue(curPage);
}

void Exporter::on_kurvaOto_toggled(bool enabled)
{
    emit kurvaOtoChanged(ui->comboVersi->currentData(CLSIDRole).toString(), enabled);
}

int KalkulasiJumlahHalaman(const QString &s)
{
    int ctr = 0;
    QStringList sl = s.split(",",Qt::SkipEmptyParts);
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

QPair<int, int> parseQty(const QString& txt) {
    int page, copy;
    QStringList splitted = txt.split("@", Qt::SkipEmptyParts);
    if(splitted.count() == 1) {
        return {splitted[0].toInt(), splitted[0].toInt()};
    } else if (splitted.count() == 2) {
        page = splitted[0].toInt();
        copy = splitted[1].toInt();
        return {page, copy};
    }
    return {1, 1};
}

#include "reporter/mainwindow.h"

namespace Reporter {
  class RMainWindow : public MainWindow {};
};

void Exporter::on_a3ReporterButton_clicked() {
  auto mw = new Reporter::RMainWindow();
  mw->setAttribute(Qt::WA_DeleteOnClose);
  mw->show();
};