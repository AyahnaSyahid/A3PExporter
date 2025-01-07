// #include "incl/corelthread.h"
// #include <qt_windows.h>
#include <combaseapi.h>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QtDebug>
#include <QSettings>
#include <QApplication>

void CorelThread::run()
{
    CoInitializeEx(0, COINIT_MULTITHREADED);
    ax = new QAxObject(getControlName());
    ax->disableEventSink();
    runMode = AlreadyRun;
    if(!ax->property("Visible").toBool())
    {
        runMode = RunByThread;
        ax->setProperty("Visible", true); 
    }
    // qDebug() << this << "Running";
    while(!stopRequested)
    {
        while (task.isEmpty()) yieldCurrentThread();
        emit processingBegin();
        runTask();
        emit processingFinished();
    }
    // qDebug() << this << "Stopping";
    ax->clear();
    delete ax;
    CoUninitialize();
}

CorelThread::CorelThread(QObject *parent) :
    QThread(parent)
{

    QSettings s(qApp->applicationDirPath() + "/conf.ini", QSettings::IniFormat, this);
    QString useVersion = s.value("CorelApplication/useVersion").toString();
    if(useVersion.isEmpty())
        setControlName("CorelDraw.Application");
    else
        setControlName(QString("CorelDraw.Apllication.%1").arg(useVersion));
}

void CorelThread::stop()
{
    stopRequested = true;
}

void CorelThread::runTask()
{
    if(task.isEmpty())
        return ;
    QString taskType = task.value("taskType").toString();

    if(ax->isNull())
        ax->clear();
    ax->setControl(getControlName());

    if(taskType == "info")
    {
        getInfo();
        task.clear();
    } else if(taskType == "export")
    {
        exportDocument();
        task.clear();
    } else if(taskType == "openSettings")
    {
        openPdfSettings();
        task.clear();
    } else if(taskType == "closeDoc")
    {
        closeCurrentDocument();
        getInfo();
        task.clear();
    }
}

void CorelThread::getInfo()
{
    QMap<QString, QVariant> res;
    bool corelOk = true;
    res.insert("getCorel", corelOk);
    corelOk = ax->setControl(controlName);
    res.insert("getCorel", corelOk);
    if(!corelOk)
    {
        emit result(res);
        stop();
        return;
    }
    QAxObject *documents = ax->querySubObject("Documents");
    if(!documents)
    {
        // here reset corel
        QAxObject *helper = ax;
        ax = new QAxObject(controlName, helper->parent());
        helper->deleteLater();
        documents = ax->querySubObject("Documents");
        if(!documents)
        {
            stop();
            return;
        }
        ax->setProperty("Visible", true);
    }
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
    }
    emit result(res);
}

void CorelThread::exportDocument()
{
    QVariantMap m;
    auto document = ax->querySubObject("ActiveDocument");
    auto pdfSettings = document->querySubObject("PdfSettings");
    pdfSettings->setProperty("PageRange", task["pageRange"]);
    pdfSettings->setProperty("TextAsCurves", task["autoCurves"]);

    QTemporaryFile tmpf("pdfEXP");
    tmpf.open();

    QString exportTarget = QString("%1\\%2").arg(task.value("dirName").toString(),
                                                 task.value("fileName").toString());
    QString tempName = QString(tmpf.fileName() + ".pdf");
    m["corelMessage"] = "exportBegin";
    emit exportMessage(m);
    document->dynamicCall("PublishToPdf(const QString&)", tempName);
    m["corelMessage"] = "exportFinish";
    emit exportMessage(m);

    bool saved = QFileInfo::exists(tempName);
    m.insert("pdfsaved", saved);
    emit exportMessage(m);
    m.remove("pdfsaved");
    QFile f(tempName);
    bool moved;
    if(QFile::exists(exportTarget)){
        moved = false;
        m.insert("errMsg", "fileExists");
    }
    else
    {
        moved = f.rename(exportTarget);
        if(!moved)
            m.insert("errMsg", f.errorString());
    }
    m.insert("pdfmoved", moved);
    m.insert("fileName", tempName);
    m.insert("exportName", exportTarget);
    emit exportMessage(m);
    tmpf.close();
}

void CorelThread::openPdfSettings()
{
    QAxObject *document = ax->querySubObject("ActiveDocument");
    if(!document)
    {
        QVariantMap m;
        m.insert("Open", "sucess");
        emit result(m);
        return;
    }
    QAxObject *settings = document->querySubObject("PdfSettings");
    settings->dynamicCall("ShowDialog()");
}

void CorelThread::closeCurrentDocument()
{
    qDebug() << "Close Document Called";
    ax->setControl(controlName);
    QAxObject *docs = ax->querySubObject("Documents");
    if(!docs)
    {
        // error
        QAxObject *help;
        help = ax;
        ax = new QAxObject(controlName);
        docs = ax->querySubObject("Documents");
        help->deleteLater();
        if(!docs)
        {
            stopRequested = true;
            return;
        }
    }
    if(docs->property("Count").toInt() > 0)
        ax->querySubObject("ActiveDocument")->dynamicCall("Close()");
}

const QMap<QString, QVariant> &CorelThread::getTask() const
{
    return task;
}

void CorelThread::setTask(const QMap<QString, QVariant> &t)
{
    if (task == t)
        return;
    task = t;
    emit taskChanged();
}

void CorelThread::resetTask()
{
    task.clear(); // TODO: Adapt to use your actual default value
}

const QString &CorelThread::getControlName() const
{
    return controlName;
}

void CorelThread::setControlName(const QString &newControlName)
{
    if (controlName == newControlName)
        return;
    controlName = newControlName;
    emit controlNameChanged();
}
