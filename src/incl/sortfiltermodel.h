#ifndef SortFilterModel_H
#define SortFilterModel_H

#include <QSortFilterProxyModel>

class SortFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SortFilterModel(QObject* =nullptr);
    ~SortFilterModel();
    
    // return QSortFilterProxyModel::rowCount
    int baseRowCount(const QModelIndex& = QModelIndex()) const;
    bool filterAcceptsRow(int sr, const QModelIndex& sp) const override;
    
    int lastPage() const;
    const int& currentPage() const { return cPage; }
    
    void setSourceModel(QAbstractItemModel*) override;
    
public slots:
    void setPage(int page);
    void setPerPage(int perPage);
    void setFilterFixedString(const QString& fx);
    void setFilterKeyColumn(int fx);

signals:
    void pageChanged(int old, int current);
    void perPageChanged(int);
    void filterKeyChanged(int k);

private:
    int cPage;
    int perPage;
    QSortFilterProxyModel* shadows;
};

#endif