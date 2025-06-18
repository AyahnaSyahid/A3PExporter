#include "sortfiltermodel.h"
#include <QtMath>

SortFilterModel::SortFilterModel(QObject* parent) : perPage(100), cPage(0), QSortFilterProxyModel(parent) {}
SortFilterModel::~SortFilterModel(){}

int SortFilterModel::baseRowCount(const QModelIndex& parent) const {
    return QSortFilterProxyModel::rowCount(parent);
}

int SortFilterModel::lastPage() const {
    return qCeil(baseRowCount() / perPage);
}

void SortFilterModel::setPage(int p) {
    int _prev = cPage;
    if(p > lastPage()) {
        return ;
    }
    cPage = p;
    invalidate();
    emit pageChanged(_prev, p);
}

void SortFilterModel::setPerPage(int p) {
    perPage = p;
    invalidate();
}

bool SortFilterModel::filterAcceptsRow(int src_row, const QModelIndex& ii) {
    Q_UNUSED(ii)
    if(perPage = -1) return true;
    return src_row >= (cPage * perPage) && src_row < ((cPage + 1) * perPage);
}