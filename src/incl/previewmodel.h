#ifndef PREVIEWMODEL_H
#define PREVIEWMODEL_H

#include <QSqlTableModel>
#include <QObject>
#include <QtDebug>

class PreviewModel : public QSqlTableModel
{
    Q_OBJECT
    int rowLimit = 100;
    int showPage = 0;

public:
    explicit PreviewModel(QObject *parent = nullptr);
    virtual QVariant data(const QModelIndex &index, int role) const override;
    QString limitStatement() const;
    inline const int &currentPage() {return showPage;}
    inline const int &currentLimit() {return rowLimit;}

public slots:
    void setLimit(const int &lim);
    void setLimit(const QString &lim);
    void setDisplayPage(const QString &page);
    void setDisplayPage(const int &page);

protected:
    virtual QString selectStatement() const override;

signals:
    void pageChanged();
    void limitChanged();

public:
    virtual void sort(int column, Qt::SortOrder ord) override;
};

#endif // PREVIEWMODEL_H
