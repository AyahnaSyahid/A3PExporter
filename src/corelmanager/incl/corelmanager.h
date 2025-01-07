#ifndef CORELMANAGER_H
#define CORELMANAGER_H

#include <QObject>
#include <QAxObject>
#include <QThread>

class CorelWorker;
class CorelManager : public QObject {
    Q_OBJECT
    static int refcount;
    static int scanBeginVersion;
    static int scanEndVersion;
    static QAxObject checker;
    static QMap<int, QPair<QString, QString>> *versions;
    QThread workerThread;

    CorelWorker *worker;

public:
    static void scan();
    static void setScanBeginVersion(int start);
    static void setScanEndVersion(int end);
    static void setScanVersion(int start, int end);
    static bool hasDocumentOpen(int version);
    static const QMap<int, QPair<QString, QString>> *installedVersions();
    CorelWorker *getWorker() const {return worker;}
    CorelManager(QObject *parent=nullptr);
    ~CorelManager();

signals:
    void initializeWorker();
    void detect(const QVariantMap&);
    void exp(const QVariantMap&);
    void sett(const QVariantMap&);
    void cldoc(const QVariantMap&);

};

class CorelWorker : public QObject {
    Q_OBJECT
    QAxObject *ax;
    QString controlName;
    bool initialized;
public:
    CorelWorker(QObject *parent=nullptr);
    ~CorelWorker();

public slots:
   void init();
   void detectDocument(const QVariantMap& task);
   void exportDocument(const QVariantMap& task);
   void openPdfSettings(const QVariantMap& task);
   void closeCurrentDocument(const QVariantMap& task);
   void setControl(const QString& n);

signals:
   void beginProcessing();
   void endProcessing();
   void result(const QVariantMap&);
   void exportMessage(const QVariantMap&);
   void controlNameChanged(const QString& old, const QString &nw);
   void taskChanged();
};

#endif // CORELMANAGER_H
