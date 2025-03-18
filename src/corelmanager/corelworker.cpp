#include "incl/corelworker.h"
#include <QAxObject>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QtDebug>
#include "combaseapi.h"

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

void CorelWorker::detectDocument(const Params& param)
{
    emit beginProcessing();
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
