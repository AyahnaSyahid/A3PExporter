#ifndef SortFilterModel_H
#define SortFilterModel_H

#include <QSortFilterProxyModel>

class SortFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SortFilterModel(QObject* =nullptr);
    ~SortFilterModel();
    
    // return QSortFilterProxyModel::rowCount
    inline int baseRowCount(const QModelIndex& = QModelIndex()) const;
    bool filterAcceptsRow(int sr, const QModelIndex& sp);
    
    int lastPage() const;
    const int& currentPage() const { return cPage; }
    
public slots:
    void setPage(int page);
    void setPerPage(int perPage);

signals:
    void pageChanged(int old, int current);

private:
    int cPage;
    int perPage;
};

#endif