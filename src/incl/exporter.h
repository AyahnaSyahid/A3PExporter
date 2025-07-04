#ifndef EXPORTER_H
#define EXPORTER_H

#include <QWidget>
#include <QFileSystemModel>
#include <QCompleter>
#include <QSharedMemory>
#include <QAbstractItemModel>
#include <QPoint>

namespace Ui { class Exporter; }

class QSettings;
class QSharedMemory;
class A3DataBase;
class QThread;
class CorelExecutor;
class Exporter : public QWidget
{
    Q_OBJECT
    QThread *executorThread;
    CorelExecutor *cx;
public:
    enum DataRole {
        ProgIDRole = Qt::UserRole,
        CLSIDRole = Qt::UserRole + 1,
    };

    Exporter(QWidget *parent = nullptr);
    ~Exporter();
    QString currentExportFolder() const;

public slots:
    void detectResultReady(const QVariantMap& vmap);
    void exportResultReady(const QVariantMap& vmap);
    void documentClosedHandler();
    void pdfSettingsResult(const QVariantMap&);
    void pdfSettingsChanged(const QVariantMap&);
    
private slots:
    void on_histTable_customContextMenuRequested(const QPoint&);
    void on_tbDet_clicked();
    void on_tbBrowse_clicked();
    void on_sideCheck_toggled(bool);
    void on_pbExport_clicked();
    void on_btPdfSetting_clicked();
    void on_lePage_textChanged(const QString&);
    void on_leExpF_textChanged(const QString&);
    // void on_pbFilter_clicked();
    void on_pbDetach_clicked();
    void on_pushButton_clicked();
    void on_comboVersi_currentIndexChanged(int);
    void on_leQty_textChanged(const QString&);
    void on_actionToggleHistory_triggered();
    void on_actionRefreshHistory_triggered();
    void on_A3PDataModel_filterChanged();
    void on_kurvaOto_toggled(bool);
    void on_actionCloseDocument_triggered();
    void handleMoveDone(const QVariantMap& vv);
    void handleMoveStart();
    void on_leFilter_textChanged(const QString& text);
    // void onMoverFailed(const QString& tplf, const QString& tdir, consr QString& fn);
    void moveExportedFile(const QVariantMap&);

signals:
    void requestDetect(const QString& clsid);
    void requestExport(const QVariantMap& param);
    void exported(const QString& fullPath);
	void requestOpenSettings(const QString& clsid);
    void kurvaOtoChanged(const QString& CLSID, bool);
    void requestClose(const QString& clsid);
    void readyToMove(const QVariantMap& res);

private:
    Ui::Exporter *ui;
    QVariantMap waitState;
};

int KalkulasiJumlahHalaman(const QString &s);
QPair<int, int> parseQty(const QString& txt);

#endif // EXPORTER_H
