#include "incl/coreldriverthread.h"
#include "combaseapi.h"
#include <QAxObject>
#include <QMutexLocker>
#include <QMutex>


CorelDriverThread::CorelDriverThread() : _isBussy(false), QObject() {}

CorelDriverThread::~CorelDriverThread() {}

bool CorelDriverThread::isBussy() {
    QMutexLocker lock(&mutex);
        return _isBussy;
}

QAxObject* CorelDriverThread::initialize_com() {
    QAxObject* ax = findChild<QAxObject*>("QAxObject");
    if(!ax) {
        CoInitializeEx(0, COINIT_MULTITHREADED);
        QAxObject* ax = new QAxObject(this);
        ax->setObjectName("AxObject");
    }
    return ax;
}

void CorelDriverThread::detect(const QString& clsid) {
    _isBussy = true;
    emit stateChanged(_isBussy);
    QAxObject* ax = initialize_com();
    ax->setControl(clsid);
    QVariantMap dres;
    int docCount = ax->querySubObject("Documents")->property("Count").toInt();
    dres.insert("docCount", docCount);
    if(docCount < 1) {
        emit detectFinished(dres);
    }
    auto active = ax->querySubObject("ActiveDocument");
    QString docPath = active->property("FilePath").toString();
    QString docFile = active->property("FileName").toString();
    QString docName = active->property("Name").toString();
    QString docId = QStringList({docPath, docFile, docName}).join(".");
    dres.insert("docId", docId);
    int pageCount = active->querySubObject("Pages")->property("Count").toInt();
    dres.insert("pageCount", pageCount);
    dres.insert(dres.cbegin(), "clsid", clsid);
    emit detectFinished(dres);
    _isBussy = false;
    emit stateChanged(_isBussy);
}

void CorelDriverThread::export_(const QString& clsid,
                               const QString& documentId,
                               const QString& exportPath,
                               const QString& exportFileName) {
    auto ax = initialize_com();
}

void CorelDriverThread::openPdfSettings(const QString& clsid) {}