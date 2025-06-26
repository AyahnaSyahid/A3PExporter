#include "incl/exporter.h"
#include "incl/a3database.h"
#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>
#include <QMetaObject>
#include <QDir>
#include <QSettings>


int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  a.setWindowIcon(QIcon(":/images/images/E.png"));
  QDir().setCurrent(a.applicationDirPath());
  A3DataBase adb;
  Exporter w;
  w.connect(&w, SIGNAL(exported(QString)), &adb, SLOT(insert(QString)));

#ifdef RELEASE_BUILD
	QSharedMemory shamem("ExporterAlive");
  if( shamem.create(13) )
  {
      w.show();
  } else {
      QMessageBox::warning(nullptr, "Error", "Ada exporter lain yang sedang di jalankan");
      w.deleteLater();
      a.quit();
      return 0;
  }
#else	
  w.show();
#endif
  return a.exec();
}
