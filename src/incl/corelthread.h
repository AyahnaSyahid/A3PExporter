#ifndef CORELTHREAD_H
#define CORELTHREAD_H

#include <QThread>
#include <QObject>
#include <QAxObject>

class CorelThread : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString controlName READ getControlName WRITE setControlName NOTIFY controlNameChanged)
    Q_PROPERTY(QMap<QString, QVariant> task READ getTask WRITE setTask RESET resetTask NOTIFY taskChanged)

public:
    enum RunMode { AlreadyRun, RunByThread };
    CorelThread(QObject *parent = nullptr);
    const QString &getControlName() const;
    const QMap<QString, QVariant> &getTask() const;
    void setControlName(const QString &newControlName);

public slots:
    void stop();
    void resetTask();
    void setTask(const QMap<QString, QVariant> &t);

private:
    QMap<QString, QVariant> task;
    QString controlName;
    void getInfo();
    void exportDocument();
    void openPdfSettings();
    void closeCurrentDocument();
    bool stopRequested = false;
    void runTask();
    RunMode runMode;
    QAxObject *ax;

signals:
    void processingBegin();
    void processingFinished();
    void result(const QMap<QString, QVariant> &res);
    void exportMessage(const QVariantMap &res);
    void controlNameChanged();
    void taskChanged();
};

#endif // CORELTHREAD_H
