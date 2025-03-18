#ifndef CorelWorker_H
#define CorelWorker_H

#include <QObject>
#include <QVariant>
#include <QMap>

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


#endif // CorelWorker_H