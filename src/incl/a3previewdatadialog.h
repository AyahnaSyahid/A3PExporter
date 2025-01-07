#ifndef A3PREVIEWDATADIALOG_H
#define A3PREVIEWDATADIALOG_H

#include "previewmodel.h"

#include <QDialog>
#include <QVariantMap>


namespace Ui {
class A3PreviewDataDialog;
}

class A3PreviewDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit A3PreviewDataDialog(QWidget *parent = nullptr);
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    ~A3PreviewDataDialog();

public slots:
    void reload();
    void manageNav();

private:
    Ui::A3PreviewDataDialog *ui;
    QString generatedFilter() const;
    void fillDateFilter();
    QStringList filterKolom;
    PreviewModel *pm;
    int lastMaxPage = 1;
    void mainTableContextMenu(const QPoint &pos);

signals:
    void filterChanged(const QString &flt = QString());
    void tableUpdated();

private slots:
    void on_tbFirst_clicked();
    void on_tbPrev_clicked();
    void on_tbNext_clicked();
    void on_tbLast_clicked();
    void on_tbPrint_clicked();
};

#endif // A3PREVIEWDATADIALOG_H
