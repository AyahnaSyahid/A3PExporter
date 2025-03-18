#ifndef EXPORTER_H
#define EXPORTER_H

#include <QWidget>
#include <QFileSystemModel>
#include <QCompleter>
#include <QSharedMemory>
#include <QAbstractItemModel>
#include <QPoint>



QT_BEGIN_NAMESPACE
namespace Ui { class Exporter; }
QT_END_NAMESPACE

namespace CustomClassNS {
    class FileSystemModel;
    class FSCompleter; 
}

class CustomClassNS::FileSystemModel : public QFileSystemModel
{
public:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

class CustomClassNS::FSCompleter : public QCompleter
{
public:
    QString pathFromIndex(const QModelIndex &idx) const override;
};


class QSettings;
class QSharedMemory;
class CorelThread;
class A3DataBase;

class Exporter : public QWidget
{
    Q_OBJECT

public:
    enum DataRole {
        ProgIdRole = Qt::UserRole,
        ClsIdRole = Qt::UserRole + 1,
    };
    Exporter(QWidget *parent = nullptr);
    ~Exporter();
    void setVersionMap(const QMap<int, QPair<QString, QString>> vmap);
    QString currentExportFolder() const;

public slots:
    void detectResultReady(const QVariantMap& vmap);
    void exportResultReady(const QVariantMap& vmap);
    
private slots:
    void on_histTable_customContextMenuRequested(const QPoint&);
    void on_tbDet_clicked();
    void on_tbBrowse_clicked();
    void on_sideCheck_toggled(bool);
    void on_pbExport_clicked();
    void on_btPdfSetting_clicked();
    void on_lePage_textChanged(const QString&);
    void on_pbFilter_clicked();
    void on_pbDetach_clicked();
    void on_pushButton_clicked();
    void on_comboVersi_currentIndexChanged(int);
    void on_leQty_textChanged(const QString&);
    void manageNavigasi();

signals:
    void requestDetect(const QString& c);
    void requestExport();

private:
    Ui::Exporter *ui;
};

#endif // EXPORTER_H
