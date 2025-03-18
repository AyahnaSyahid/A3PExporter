#ifndef CORELMANAGER_H
#define CORELMANAGER_H

#include <QObject>
#include <QPair>
#include <QThread>
#include <QVariantMap>


class CorelManager : public QObject {
    Q_OBJECT
	QThread *workerThread;

public:
	CorelManager(QObject *parent=nullptr);
    ~CorelManager();
	QMap<int, QPair<QString, QString>> versionMap(int end=60, int start=22) const;

public slots:
    void detectCurrentDocument(const QString& clsid);
    void exportDocument(const QString& clsid,
                        const QString& documentId,
                        const QString& exportPath,
                        const QString& exportFileName);
    void openPdfSettings(const QString& clsid);
    void closeDocument(const QString& clsid,
                       const QString& documentId);
signals:
    void detectResult(const QString& clsid, const QVariantMap& vmap);
    void exportResult(const QString& clsid, const QVariantMap& vmap);
};

#endif // CORELMANAGER_H
