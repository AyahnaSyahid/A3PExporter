#include "incl/exporter.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/images/E.png"));
    Exporter w;
    if(w.isFirstInstance())
    {
        w.show();
    } else
    {
        QMessageBox::warning(nullptr, "Error", "Ada exporter lain yang sedang di jalankan");
        w.deleteLater();
        return 0;
    }
    return a.exec();
}
