#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dirscanner.h"

#include <QMainWindow>
#include <QList>
#include <QMap>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QSettings;
class ExcelWriter;
class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(QString execDir READ execDir WRITE setExecDir NOTIFY execDirChanged)
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    inline const QString &execDir() const {return _ed;} ;
    void setExecDir(const QString &ed);
    QList<QMap<QString, QVariant>> modelData;

signals:
    void execDirChanged(const QString &c);
    void modelDataTruncated();
    void modelDataChanged();
    void directoryScanFinished();

private slots:
    void updateStBarSum();
    void on_pilihBtn_released();
    void on_simpanBtn_released();
    void on_mulaiBtn_released();
    void scanDone(custom_type result);
    void writeStatus(const QString &status);
    void on_listView_customContextMenuRequested(const QPoint &pos);

private:
    QString _ed;
    Ui::MainWindow *ui;
    ExcelWriter *laporan;
    QSettings *setting;

};

#endif // MAINWINDOW_H
