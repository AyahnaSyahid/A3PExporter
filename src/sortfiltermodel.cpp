#include "incl/sortfiltermodel.h"
#include <QtMath>
#include <QtDebug>

SortFilterModel::SortFilterModel(QObject* parent)
    : perPage(100), cPage(0), shadows(new QSortFilterProxyModel(this)), QSortFilterProxyModel(parent)
{
    connect(this, SIGNAL(filterKeyChanged(int)), shadows, SLOT(setFilterKeyColumn(int)));
}
SortFilterModel::~SortFilterModel(){}

int SortFilterModel::baseRowCount(const QModelIndex& parent) const {
    return shadows->rowCount();
}

int SortFilterModel::lastPage() const {
    qDebug() << "BaseRowCount : " << baseRowCount() << " | MeRowCount : " << rowCount();
    if(perPage < 0) return 0;
    return qCeil(baseRowCount() / perPage);
}

void SortFilterModel::setFilterFixedString(const QString& pattern) {
    shadows->setFilterFixedString(pattern);
    QSortFilterProxyModel::setFilterFixedString(pattern);
}

void SortFilterModel::setFilterKeyColumn(int c) {
    shadows->setFilterKeyColumn(c);
    QSortFilterProxyModel::setFilterKeyColumn(c);
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
    emit perPageChanged(p);
}

bool SortFilterModel::filterAcceptsRow(int src_row, const QModelIndex& ii) const {
    bool ok = QSortFilterProxyModel::filterAcceptsRow(src_row, ii);
    if(!ok) return false;
    if(perPage < 0) {
        return ok;
    } else {
        return src_row >= (cPage * perPage) && src_row < ((cPage + 1) * perPage);
    }
    return false;
}

void SortFilterModel::setSourceModel(QAbstractItemModel* model) {
    shadows->setSourceModel(model);
    QSortFilterProxyModel::setSourceModel(model);
}