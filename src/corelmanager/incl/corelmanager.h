#ifndef CORELMANAGER_H
#define CORELMANAGER_H

#include <QObject>
#include <QPair>
#include <QThread>
#include <QVariantMap>

class CorelDriverThread;
class CorelManager : public QObject {
    Q_OBJECT
	QThread *workerThread;
	CorelDriverThread *cdt;
public:
	CorelManager(QObject *parent=nullptr);
    ~CorelManager();
	QMap<int, QPair<QString, QString>> versionMap(int end=60, int start=22) const;

public slots:
    void detectDocument(const QString& clsid);
    void exportDocument(const QString& clsid,
                        const QString& documentId,
                        const QString& exportPath,
                        const QString& exportFileName);
    void openPdfSettings(const QString& clsid);
    void closeDocument(const QString& clsid,
                       const QString& documentId);
signals:
    void detectFinish(const QString& clsid, const QVariantMap& vmap);
    void exportFinish(const QString& clsid, const QVariantMap& vmap);
};

#endif // CORELMANAGER_H
