#include "incl/exporter.h"
#include "ui/ui_exporter.h"
#include "incl/a3previewdatadialog.h"
#include "incl/tentangaplikasi.h"
#include "incl/a3database.h"
#include "corelmanager/incl/corelexecutor.h"
#include "incl/models.h"

#include <QCompleter>
#include <QRegularExpression>
#include <QAbstractItemModel>
#include <QRegularExpressionValidator>
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
    ui->setupUi(this);
    QSettings* glb = new QSettings("conf.ini", QSettings::IniFormat, this);
    glb->setObjectName("settings");
    ui->comboVersi->clear();
    
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
    
    A3PDataModel* model = new A3PDataModel(this);
    model->setObjectName("A3PDataModel");
    model->setMaxRow(200);
    model->setFilter();
    ui->histTable->setModel(model);
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
    connect(model, &QAbstractItemModel::rowsRemoved, model, &A3PDataModel::submit);
    connect(ui->cbxViewRange,&QComboBox::currentTextChanged, model, &A3PDataModel::toggleShowMode);
    connect(model, &A3PDataModel::currentPageChanged, this, &Exporter::manageNavigasi);
    connect(model, &A3PDataModel::filterChanged, this, &Exporter::manageNavigasi);
    connect(ui->btNext, &QToolButton::clicked, model, &A3PDataModel::nextPage);
    connect(ui->btPrev, &QToolButton::clicked, model, &A3PDataModel::prevPage);
    connect(ui->btLast, &QToolButton::clicked, model, &A3PDataModel::lastPage);
    connect(ui->btFirst, &QToolButton::clicked, model, &A3PDataModel::firstPage);
    
    ui->sideCheck->setEnabled(false);
    ui->sideCheck->setChecked(false);
    
    cx->moveToThread(executorThread);
    
    connect(executorThread, &QThread::started, cx, &CorelExecutor::init);
    connect(this, &Exporter::requestDetect, cx, &CorelExecutor::runDetect);
    connect(cx, &CorelExecutor::detectResult, this, &Exporter::detectResultReady);
    
    executorThread->start();
}

Exporter::~Exporter()
{
    QSettings* glb = findChild<QSettings*>("settings");
    glb->sync();
    delete ui;
}

void Exporter::detectResultReady(const QVariantMap& res)
{
    waitState[QString("%1_detect").arg(res["controlName"].toString())] = false;
    if(!res["state"].toBool()) {
        QMessageBox::information(this, "Kesalahan", QString("Error :\n%1").arg(res["stateMessage"].toString()));
        return ;
    }
    ui->leDoc->clear();
    ui->leBahan->clear();
    ui->leQty->clear();
    ui->leKlien->clear();
    ui->lePage->clear();
    ui->txKet->clear();
    if(res["documentCount"].toInt() < 1) {
        ui->leDoc->setText("Tidak ada file");
    } else {
        ui->leDoc->setText(res["fileName"].toString().isEmpty() ? res["vbaName"].toString() : res["fileName"].toString());
        if(!res["filePath"].toString().isEmpty()) {
            ui->leKlien->setText(QDir(res["filePath"].toString()).dirName());
        }
        ui->lePage->setText(res["pageCount"].toInt() > 1 ? QString("1-%1").arg(res["pageCount"].toInt()) : "1");
    }
    qDebug() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    qDebug() << res;
    qDebug() << "";
}

void Exporter::exportResultReady(const QVariantMap& res)
{
    qDebug() << res;
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
    emit requestDetect(controlName);
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
    emit ui->lePage->textChanged(ui->lePage->text());
}

void Exporter::on_pbExport_clicked() {}
void Exporter::manageNavigasi() {}
void Exporter::on_lePage_textChanged(const QString& txt)
{
    int kalk = KalkulasiJumlahHalaman(txt);
    int second, lastCopy;
    second = parseQty(ui->leQty->text()).second;
    lastCopy = (second == 0) ? 1 : second;
    if(kalk > 0)
    { 
        if(!(kalk % 2))
        {
            ui->sideCheck->setEnabled(true);
            if(ui->sideCheck->isChecked())
                if(kalk / 2 > 1)
                    ui->leQty->setText(QString("%1@%2").arg(kalk / 2).arg(lastCopy));
                else
                    ui->leQty->setText(QString("%1").arg(kalk / 2));
            else
                ui->leQty->setText(QString("%1@%2").arg(kalk).arg(lastCopy));
        }
        else
        {
            ui->sideCheck->setEnabled(false);
            ui->sideCheck->setChecked(false);
            if(kalk == 1)
                ui->leQty->setText("1");
            else
                ui->leQty->setText(QString("%1@%2").arg(kalk).arg(lastCopy));
        }
    }
}

void Exporter::on_leQty_textChanged(const QString& txt) {
    auto kl = parseQty(ui->leQty->text());
    if (kl == QPair<int, int>{0, 0}) {
        ui->leQty->setToolTip("");
    } else {
        QString tbl(R"(<style>table { border-spacing: 10px; } td { padding: 2px; }</style>
                    <table><tr><td align='right'><b>Lembar :</b></td><td align='right'>%2</td></tr>
                    <tr><td align='right'><b>Klik :</b></td><td align='right'>%1</td></tr></table>)");
        if (ui->sideCheck->isChecked()) {
            ui->leQty->setToolTip(tbl
                        .arg(locale().toString(kl.first * 2 * kl.second))
                        .arg(locale().toString(kl.first * kl.second)));
        } else {
            ui->leQty->setToolTip(tbl
                        .arg(locale().toString(kl.first * kl.second))
                        .arg(locale().toString(kl.first * kl.second)));
        }
    }
}

// void Exporter::requestExport()
// {
    // QVariantMap m;
    // m.insert("taskType", "export");
    // QString targetFileName = QString("%1_%2_%3_%4")
            // .arg(ui->leKlien->text().simplified().remove("_").toUpper(),
                 // ui->leFile->text().simplified().remove("_").toUpper(),
                 // ui->leBahan->text().simplified().remove("_").toUpper(),
                 // ui->leQty->text().simplified().remove("_").toUpper());
    // if(ui->sideCheck->isChecked())
        // targetFileName += "_BB";
    // if(ui->txKet->toPlainText().simplified().toUpper().length())
        // targetFileName += QString("_%1").arg(ui->txKet->toPlainText().simplified().toUpper());
    // targetFileName += ".pdf";
    // m.insert("autoCurves", ui->kurvaOto->isChecked());
    // m.insert("pageRange", ui->lePage->text());
    // m.insert("fileName", targetFileName);
    // m.insert("dirName", ui->leExpF->text());
    // auto *mgr = SingletonNS::manager;
    // m["corelVersion"] = ui->comboVersi->currentData(Qt::UserRole + 1);
    // emit mgr->exp(m);
// }

// void Exporter::toggleExportButton()
// {
    // exportName = "";
    // if(ui->pbExport->isEnabled())
        // ui->pbExport->setDisabled(true);
    // if(ui->leExpF->text().isEmpty())
        // return;
    // else {
        // auto fexist = QFileInfo::exists(ui->leExpF->text());
        // if(!fexist)
            // return;
        // if(!QFileInfo(ui->leExpF->text()).isDir())
            // return;
    // }
    // exportName += ui->leExpF->text();
    // exportName += "/";

    // if(ui->leKlien->text().isEmpty())
        // return;
    // exportName += ui->leKlien->text();

    // if(ui->leFile->text().isEmpty())
        // return;
    // exportName += "_";
    // exportName += ui->leFile->text();

    // if(ui->leBahan->text().isEmpty())
        // return ;
    // exportName += "_";
    // exportName += ui->leBahan->text();

    // if(ui->leQty->text().isEmpty())
        // return;
    // exportName += "_";
    // exportName += ui->leQty->text();

    // if(ui->sideCheck->isChecked())
    // {
        // exportName += "_BB";
    // }

    // QString ket = ui->txKet->toPlainText().simplified();
    // if(ket.length())
        // exportName += QString("_%1").arg(ket);
    // ui->pbExport->setEnabled(true);
// }

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
    tconMenu.addAction(delRow);
    tconMenu.addAction(edt);

    QPoint gp = ui->histTable->mapToGlobal(pos);
    tconMenu.exec(gp);
}

void Exporter::on_btPdfSetting_clicked()
{
    QVariantMap m;
    m.insert("taskType", "openSettings");
    m["corelVersion"] = ui->comboVersi->currentData(Qt::UserRole + 1);
    // auto *mgr = SingletonNS::manager;
    // emit mgr->sett(m);
}

// void Exporter::manageNavigasi()
// {
    // A3PDataModel *model = qobject_cast<A3PDataModel*>(ui->histTable->model());
    // if (model) {
		// ui->btNext->setEnabled(model->hasNextPage());
		// ui->btPrev->setEnabled(model->hasPrevPage());
		// ui->btFirst->setEnabled(model->hasPrevPage());
		// ui->btLast->setEnabled(model->hasNextPage());
		// ui->spHalaman->setValue(model->currentPage() + 1);
		// ui->spHalaman->setSuffix(QString("/%1").arg(model->maxPage()));
	// }
// }

void Exporter::on_pbFilter_clicked()
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
				" (SELECT CASE WHEN keterangan IS NULL THEN \"\" ELSE keterangan END ) || \" \" || "
				"(jkertas * jkopi * sisi) LIKE \"%%1%\"",
				ftext = ui->leFilter->text().simplified();
		if(!ftext.isEmpty())
			flt = flt.arg(ftext);
		else
			flt = "";
		mod->setFilter(flt);
	}    // qDebug() << mod->filter();
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

// void Exporter::updateExportFolder(const QString &name)
// {
    // glb->setValue("Exporter/lastExportFolder", name);
// }

void Exporter::on_pushButton_clicked()
{
    TentangAplikasi *ta = new TentangAplikasi();
    connect(ta, SIGNAL(destroyed()), ta, SLOT(deleteLater()));
    ta->show();
}

void Exporter::on_comboVersi_currentIndexChanged(int index)
{
    // QString message;
    // QSettings* glb = findChild<QSettings*>("settings");
    // QString old = glb->value("CorelApplication/useVersion").toString(),
            // _new = ui->comboVersi->itemText(index);
            // message = "Pastikan anda telah menginstall Corel Versi %1 "
                      // "Lanjutkan perubahan Corel atau tetap menggunakan Versi %2";
    
    // int result = QMessageBox::information(this, "Konfirmasi Corel yang digunakan",
                                          // message.arg(_new, old), QMessageBox::Ok, QMessageBox::No);
    // if(result == QMessageBox::No) {
        // ui->comboVersi->setCurrentText(old);
        // QMessageBox::information(this, "Info", "Dibatalkan");
        // return;
    // }
    // glb->setValue("CorelApplication/useVersion", _new);
    // glb->sync();
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
    return {0, 0};
}