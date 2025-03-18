#include "incl/corelexecutor.h"
#include "combaseapi.h"
#include <QAxObject>

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
    if(!ok) {
        res.insert("state", false);
        res.insert("stateMessage", "setControl Gagal (false)");
        emit detectResult(res);
        emit endDetect(CLSID);
        return;
    }
    res.insert("state", true);
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

void CorelExecutor::runExport(const QString& clsid, const QString& docid, const QString& exportPath, const QString& exportFile) {}
void CorelExecutor::openSettings(const QString& clsid) {}