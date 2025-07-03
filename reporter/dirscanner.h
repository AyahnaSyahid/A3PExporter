#ifndef DirScanner_H
#define DirScanner_H

#include <QThread>

typedef QList<QMap<QString,QVariant>> custom_type;
Q_DECLARE_METATYPE(custom_type);

class DirScanner : public QThread
{
    Q_OBJECT
public:
    DirScanner(QObject* =nullptr);
    ~DirScanner();
    void setScanDir(const QString &dirname);
    
protected:
    void run() override;

signals:
    void result(const custom_type R);
    void errorOcurs(QString msg);
    void sendMessages(const QString &s);

private:
    QString dirName;
};

#endif