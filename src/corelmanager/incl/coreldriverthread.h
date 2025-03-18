#ifndef CORELDRIVERTHREAD
#define CORELDRIVERTHREAD

#include <QThread>
#include <QMutex>
#include <QVariantMap>

class QAxObject;
class CorelDriverThread : public QObject {
    Q_OBJECT
    bool _isBussy;
    QMutex mutex;
    
public:
    CorelDriverThread();
    ~CorelDriverThread();
    bool isBussy();

public slots:
    void detect(const QString& clsid);
    void export_(const QString& clsid,
                const QString& documentId,
                const QString& exportPath,
                const QString& exportFileName);
    void openPdfSettings(const QString& clsid);

signals:
    void stateChanged(bool bussy);
    void detectFinished(const QVariantMap vmap);
    void exportFinished(const QVariantMap vmap);

private:
    QAxObject* initialize_com();
};


#endif // CORELDRIVERTHREAD