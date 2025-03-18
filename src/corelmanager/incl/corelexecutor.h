#ifndef CorelExecutorH
#define CorelExecutorH

#include <QObject>

class QAxObject;
class CorelExecutor : public QObject{
    QAxObject* _ax;
    
public:
    CorelExecutor();
    ~CorelExecutor();
    QAxObject* initialize();
    
    
public slots:
    void runExport(const QString& clsid, const QString& docid, const QString& exportPath, const QString& exportFile);
    void runDetect(const QString& clsid);
    void openSettings(const QString& clsid);
    
};
#endif