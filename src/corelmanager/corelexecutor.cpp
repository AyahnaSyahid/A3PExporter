#include "incl/corelexecutor.h"
#include "combaseapi.h"
#include <QAxObject>
#include <QTemporaryFile>
#include <QSettings>
#include <QtDebug>

void printExc(int i, QString s1, QString s2, QString s3) {
    qDebug() << i;
    qDebug() << s1;
    qDebug() << s2;
    qDebug() << s3;
}

CorelExecutor::CorelExecutor()
 :  _ax(nullptr), QObject() {}
 
CorelExecutor::~CorelExecutor() {
    if(_ax) {
        _ax->clear();
        _ax->deleteLater();
    }
    CoUninitialize();
}

void CorelExecutor::init() {
    initialize();
}

QAxObject* CorelExecutor::initialize() {
    if(_ax) {
        return _ax;
    }
    CoInitializeEx(0, COINIT_MULTITHREADED);
    _ax = new QAxObject();
    return _ax;
}

void CorelExecutor::runDetect(const QString& CLSID) {
    auto com = initialize();
    emit beginDetect(CLSID);
    QVariantMap res;
    res.insert("controlName", CLSID);
    bool ok = com->setControl(CLSID);
    res.insert("state", true);
    if(!ok) {
        res.insert("state", false);
        res.insert("stateMessage", "setControl Gagal (false)");
        emit detectResult(res);
        emit endDetect(CLSID);
        return;
    }
    // connect(com, &QAxObject::exception, &printExc);
    res.insert("stateMessage", QString("setControl Success (%1)").arg(CLSID));
    auto docs = com->querySubObject("Documents");
    int docCount = docs->property("Count").toInt();
    res.insert("documentCount", docCount);
    if(docCount < 1) {
        com->querySubObject("AppWindow")->setProperty("WindowState", 3);
        com->setProperty("Visible", true);
        emit detectResult(res);
        emit endDetect(CLSID);
        return;
    }
    auto doc = com->querySubObject("ActiveDocument");
    res.insert("filePath", doc->property("FilePath"));
    res.insert("fileName", doc->property("FileName"));
    res.insert("vbaName", doc->property("Name"));
    res.insert("pageCount", doc->querySubObject("Pages")->property("Count"));
    emit detectResult(res);
    emit endDetect(CLSID);
}

void CorelExecutor::runExport(const QVariantMap& params) {
    QString CLSID = params["CLSID"].toString();
    auto com = initialize();
    emit beginExport(CLSID);
    QVariantMap res;
    res.insert("controlName", CLSID);
    bool ok = com->setControl(CLSID);
    res.insert("state", ok);
    if(!ok) {
        res.insert("stateMessage", "setControl Gagal (false)");
        emit exportResult(res);
        emit endExport(CLSID);
        return;
    }
    /* docId masih belum bisa di implementasi untuk sekarang gunakan ActiveDocument
    */
    // connect(com, &QAxObject::exception, &printExc);
    int docCount = com->querySubObject("Documents")->property("Count").toInt();
    if(docCount < 1) {
        res["state"] = false;
        res.insert("stateMessage", "Tidak menemukan Dokumen terbuka");
        emit exportResult(res);
        emit endExport(CLSID);
        return;
    }
    auto act = com->querySubObject("ActiveDocument");
    auto pdfs = act->querySubObject("PDFSettings");
    if(PDFSettingsBag.count() > 1) {
        pdfs->setPropertyBag(PDFSettingsBag);
        pdfs->setProperty("PageRange", params["PageRange"]);
        PDFSettingsBag.clear();
    } else {
        QSettings* glb = sender()->findChild<QSettings*>("settings");
        auto eset = glb->value("PDFSettings");
        if(eset.isValid()) {
            pdfs->setPropertyBag(eset.toMap());
            pdfs->setProperty("PageRange", params["PageRange"]);
        }
    }
    QTemporaryFile tfile("ExporterTemp-XXXXXXXXXXXX.pdf");
    tfile.setAutoRemove(false);
    tfile.open();
    res.insert("tempFile", tfile.fileName());
    tfile.close();
    act->dynamicCall("PublishToPdf(const QString&)", tfile.fileName());
    res["exportPath"] = params["exportPath"];
    res["exportName"] = params["exportName"];
    
    emit exportResult(res);
}

void CorelExecutor::openSettings(const QString& CLSID) {
    auto com = initialize();
    QVariantMap res;
    res.insert("controlName", CLSID);
    bool ok = com->setControl(CLSID);
    res.insert("state", ok);
    if(!ok) {
        res.insert("stateMessage", "setControl Gagal (false)");
        return;
    }
    // connect(com, SIGNAL(exception(int, QString, QString, QString)), &printExc);
    res.insert("stateMessage", "setControl Success (true)");
    int docCount = com->querySubObject("Documents")->property("Count").toInt();
    res.insert("documentCount", docCount);
    if(docCount < 1) {
        res["state"] = false;
        res["stateMessage"] = "Tidak ada document terbuka";
        emit pdfSettingsResult(res);
        return;
    }
    auto act = com->querySubObject("ActiveDocument");
    emit pdfSettingsOpen(CLSID);
    auto pset = act->querySubObject("PDFSettings");
    bool saved = pset->dynamicCall("ShowDialog()").toBool();
    emit pdfSettingsClosed(CLSID);
    emit pdfSettingsResult(res);
    if(saved) {
        emit pdfSettingsChanged(pset->propertyBag());
    } else {
        PDFSettingsBag = pset->propertyBag();
    }
}