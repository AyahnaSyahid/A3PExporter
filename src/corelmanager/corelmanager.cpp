#include "incl/corelmanager.h"
#include "combaseapi.h"
#include <QVector>
#include <QMap>

#include <QFileInfo>
#include <QTemporaryFile>
#include <QtDebug>

int CorelManager::scanBeginVersion = 14;
int CorelManager::scanEndVersion = 23;
int CorelManager::refcount = 0;

QAxObject CorelManager::checker;

QMap<int, QPair<QString, QString>> *CorelManager::versions = nullptr;

void CorelManager::scan()
{
    if(!versions) {
        versions = new QMap<int, QPair<QString, QString>>;
        versions->clear();
        for(int i=scanBeginVersion; i<=scanEndVersion; ++i) {
            QString ProgID = QString("CorelDRAW.Application.%1").arg(i);
            CLSID clsid;
            auto ret = CLSIDFromString(ProgID.toStdWString().c_str(), &clsid);
            if(ret == S_OK) {
                wchar_t buffer[64];
                StringFromGUID2(clsid, buffer, 64);
                versions->insert(i, {ProgID, QString::fromWCharArray(buffer)});
            }
        }
    }
}

void CorelManager::setScanBeginVersion(int start) {
    if(scanBeginVersion != start) {
        scanBeginVersion = start;
        scan();
    }
}

void CorelManager::setScanEndVersion(int end) {
    if(scanEndVersion != end) {
        scanBeginVersion = end;
        scan();
    }
}

void CorelManager::setScanVersion(int start, int end) {
    setScanBeginVersion(start);
    setScanEndVersion(end);
}

bool CorelManager::hasDocumentOpen(int version)
{
    if(!versions->contains(version)) return false;
    checker.setControl(versions->value(version).second);
    if(checker.isNull())
        checker.setControl(versions->value(version).first);
    if(checker.isNull()) return false;
    return checker.querySubObject("Documents")->property("Count").toInt() > 0;
}

const QMap<int, QPair<QString, QString> > *CorelManager::installedVersions()
{
    if(!versions) scan();
    return versions;
}

CorelManager::CorelManager(QObject *parent)
    : QObject(parent), worker(new CorelWorker)
{
    scan();
    refcount++;
    worker->moveToThread(&workerThread);
    connect(this, &CorelManager::detect, worker, &CorelWorker::detectDocument);
    connect(this, &CorelManager::exp, worker, &CorelWorker::exportDocument);
    connect(this, &CorelManager::sett, worker, &CorelWorker::openPdfSettings);
    connect(this, &CorelManager::cldoc, worker, &CorelWorker::closeCurrentDocument);
    connect(this, SIGNAL(initializeWorker()), worker, SLOT(init()));
    workerThread.start();
    emit initializeWorker();
}

CorelManager::~CorelManager()
{
    worker->deleteLater();
    workerThread.quit();
    workerThread.wait();
    refcount--;
    if(refcount < 1) {
        refcount = 0;
        delete versions;
        versions = nullptr;
    }
}

CorelWorker::CorelWorker(QObject *parent)
    : QObject(parent),
      controlName("CorelDRAW.Application"),
      initialized(false)
{}

CorelWorker::~CorelWorker()
{
    ax->clear();
    ax->deleteLater();
    CoUninitialize();
}

void CorelWorker::init()
{
    CoInitializeEx(0, COINIT_MULTITHREADED);
    ax = new QAxObject;
    initialized = true;
}

void CorelWorker::detectDocument(const QVariantMap& task)
{
    emit beginProcessing();
    qDebug() << task;  
    if(!initialized) init();
    QVariantMap res;
    bool corelOk = ax->setControl(task["corelVersion"].toString());
    qDebug() << "COREL OK :" << corelOk;
    res.insert("getCorel", corelOk);
    if(!corelOk)
    {
        emit result(res);
        emit endProcessing();
        ax->clear();
        return;
    }
    auto *documents = ax->querySubObject("ActiveDocument");
    res.insert("corelVersion", ax->property("VersionMajor"));
    res.insert("getDocuments", documents->property("Count").toInt() >= 1 ? true : false);
    if(res.value("getDocuments").toBool())
    {
        auto currentDocument = ax->querySubObject("ActiveDocument");
        auto pages = currentDocument->querySubObject("Pages");
        auto pdfSettings = currentDocument->querySubObject("PdfSettings");

        pdfSettings->setProperty("Author", "A3PExporter");
        pdfSettings->setProperty("PublishRange", 3);
        pdfSettings->setProperty("BitmapCompression", 2);
        pdfSettings->setProperty("DownsampleColor", true);
        pdfSettings->setProperty("DownsampleGray", true);
        pdfSettings->setProperty("DownsampleMono", true);
        pdfSettings->setProperty("ColorResolution", 300);
        pdfSettings->setProperty("GrayResolution", 600);
        pdfSettings->setProperty("MonoResolution", 600);
        pdfSettings->setProperty("OutputSpotColorsAs", 1);

        QVariant fileName, filePath, pageCount, docSaved;
        fileName = currentDocument->property("FileName");
        filePath = currentDocument->property("FilePath");
        docSaved = (!filePath.toString().isEmpty());
        pageCount = pages->property("Count");
        res.insert("name", currentDocument->property("Name"));
        res.insert("docSaved", docSaved);
        res.insert("fileName", fileName);
        res.insert("filePath", filePath);
        res.insert("pageCount", pageCount);
        currentDocument->clear();
        pages->clear();
        pdfSettings->clear();
    }
    emit result(res);
    emit endProcessing();
}

void CorelWorker::exportDocument(const QVariantMap &task)
{
    emit beginProcessing();
    if(!initialized) init();
    QVariantMap res;
    bool corelOk = ax->setControl(task["corelVersion"].toString());
    res.insert("getCorel", corelOk);
    if(!corelOk)
    {
        emit result(res);
        emit endProcessing();
        ax->clear();
        return;
    }
    auto *document = ax->querySubObject("ActiveDocument");
    auto *pdfSettings = document->querySubObject("PdfSettings");
    pdfSettings->setProperty("PageRange", task["pageRange"]);
    pdfSettings->setProperty("TextAsCurves", task["autoCurves"]);

    QTemporaryFile tmpf("pdfEXP");
    tmpf.open();

    QString exportTarget = QString("%1\\%2").arg(task.value("dirName").toString(),
                                                 task.value("fileName").toString());
    QString tempName = QString(tmpf.fileName() + ".pdf");
    res["corelMessage"] = "exportBegin";
    emit exportMessage(res);
    document->dynamicCall("PublishToPdf(const QString&)", tempName);
    res["corelMessage"] = "exportFinish";
    emit exportMessage(res);

    bool saved = QFileInfo::exists(tempName);
    res.insert("pdfsaved", saved);
    emit exportMessage(res);
    res.remove("pdfsaved");
    QFile f(tempName);
    bool moved;
    if(QFile::exists(exportTarget)){
        moved = false;
        res.insert("errMsg", "fileExists");
    }
    else
    {
        moved = f.rename(exportTarget);
        if(!moved)
            res.insert("errMsg", f.errorString());
    }
    res.insert("pdfmoved", moved);
    res.insert("fileName", tempName);
    res.insert("exportName", exportTarget);
    emit exportMessage(res);
    tmpf.close();
    document->clear();
    pdfSettings->clear();
    ax->clear();
    emit endProcessing();
}

void CorelWorker::openPdfSettings(const QVariantMap &task)
{
    emit beginProcessing();
    if(!initialized) init();
    QVariantMap res;
    bool corelOk = ax->setControl(task["corelVersion"].toString());
    res.insert("getCorel", corelOk);
    if(!corelOk)
    {
        emit result(res);
        emit endProcessing();
        ax->clear();
        return;
    }
    auto *document = ax->querySubObject("ActiveDocument");
    auto *pdfSettings = document->querySubObject("PdfSettings");
    pdfSettings->dynamicCall("ShowDialog()");
    pdfSettings->clear();
    ax->clear();
    emit endProcessing();
}

void CorelWorker::closeCurrentDocument(const QVariantMap &task)
{
    emit beginProcessing();
    if(!initialized) init();
    bool corelOk = ax->setControl(task["corelVersion"].toString());
    if(!corelOk) {
        emit endProcessing();
    } else {
        auto *cdoc = ax->querySubObject("ActiveDocument");
        if (cdoc)
        {
            cdoc->dynamicCall("Close()");
        }
        emit endProcessing();
    }
    detectDocument(task);
}

void CorelWorker::setControl(const QString &n)
{
    if(n != controlName) {
        const auto old = controlName;
        controlName = n;
        emit controlNameChanged(old, n);
    }
}
