#include "incl/exporter.h"
#include "incl/a3database.h"
#include "corelmanager/incl/corelmanager.h"
#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>
#include <QMetaObject>
#include <QDir>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/images/E.png"));
	QDir().setCurrent(a.applicationDirPath());
    
	A3DataBase adb;
    CorelManager cm;
    Exporter w;
    w.setVersionMap(cm.versionMap(17, 50));
    QSharedMemory shamem("ExporterAlive");    
    if( shamem.create(13) )
    {
        w.show();
    } else {
        QMessageBox::warning(nullptr, "Error", "Ada exporter lain yang sedang di jalankan");
        w.deleteLater();
        return 0;
    }
    
    return a.exec();
}
