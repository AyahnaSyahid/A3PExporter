#ifndef PrintPreviewModel_H
#define PrintPreviewModel_H

#include <QSortFilterProxyModel>

class PrintPreviewModel: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PrintPreviewModel(QObject* =nullptr);
    ~PrintPreviewModel();
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariantList rowData(const int &row) const;
public slots:
    void setFilterWildCard(const QString &wc);
};

#endif