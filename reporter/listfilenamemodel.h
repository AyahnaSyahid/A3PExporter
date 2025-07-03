#ifndef ListFileNameModel_H
#define ListFileNameModel_H

#include <QSortFilterProxyModel>

class ListFileNameModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ListFileNameModel(QObject* =nullptr);
    ~ListFileNameModel();
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

public slots:
    void setFilterWildCard(const QString &wc);
};

#endif