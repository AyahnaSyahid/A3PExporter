#ifndef CorelExecutorH
#define CorelExecutorH

#include <QObject>
#include <QVariant>

class QAxObject;
class CorelExecutor : public QObject{
    Q_OBJECT
    QAxObject* _ax;
    
public:
    CorelExecutor();
    ~CorelExecutor();
    void enableAutoCurve(const QString&, bool);
    
public slots:
    void init();
    void runExport(const QVariantMap& params);
    void runDetect(const QString& clsid);
    void openSettings(const QString& clsid);

signals:
    void beginDetect(const QString& CLSID);
    void endDetect(const QString& CLSID);
    void beginExport(const QString& CLSID);
    void endExport(const QString& CLSID);
    void detectResult(const QVariantMap& vm);
    void exportResult(const QVariantMap& vm);
    void pdfSettingsOpen(const QString& s);
    void pdfSettingsClosed(const QString& s);
    void pdfSettingsChanged(const QVariantMap&);
    void pdfSettingsResult(const QVariantMap&);

private:
    QAxObject* initialize();
    QVariantMap PDFSettingsBag;
};
#endif