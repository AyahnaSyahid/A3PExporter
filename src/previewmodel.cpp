#include "incl/previewmodel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QtMath>
#include <QtDebug>

PreviewModel::PreviewModel(QObject* parent)
    : __max_page(1), QSqlQueryModel(parent) {
  setProperty("currentPage", 1);
  setProperty("filterDate", "all");
  setProperty("rowLimit", 100);
  setProperty("filterBy", "all");
  setProperty("filterValue", "%");
};

PreviewModel::~PreviewModel() {}

void PreviewModel::updateQuery() {
  // kalkulasi total page berdasarkan property yang telah ditentukan
  int _cPage = property("currentPage").toInt(),
      _cLimit = property("rowLimit").toInt();
  QString _cFilterDate(property("filterDate").toString()),
          _cFilterBy(property("filterBy").toString()),
          _cFilterValue(property("filterValue").toString());
  
  QString countQuery("SELECT COUNT(*) from a3pdata"),
          modelQuery("SELECT * from a3pdata");
  
  QStringList whereClauses;
  
  if(_cFilterDate != "all") {
    whereClauses << "DATE(created) = :filterDate";
  }
  
  if(!_cFilterValue.isEmpty() && !_cFilterValue.replace("%", "").isEmpty()) {
    if(_cFilterBy != "all") {
      whereClauses << QString("%1 LIKE :filterValue").arg(_cFilterBy);
    } else {
      whereClauses << QString("klien || file || bahan || COALESCE(keterangan, ' ') LIKE :filterValue");
    }
  }
  
  if(whereClauses.count()) {
    countQuery += " WHERE " + whereClause.join(" AND ");
    modelQuery += " WHERE " + whereClause.join(" AND ");
  }
  
  if(_cLimit > 0) {
    modelQuery += QString(" LIMIT :limit OFFSET :offset;");
  }
  
  QSqlQuery q;
  q.prepare(countQuery);
  if(countQuery.contains(":filterValue")) {
    q.bindValue(":filterValue", _cFilterValue);
  }
  if(countQuery.contains(":filterDate")) {
    q.bindValue(":filterDate", _cFilterDate);
  }
  
  if(q.exec() && q.next()) {
    int rows = q.value(0).toInt();
  } else {
    qDebug() << "Error :" << q.lastError().text();
    return;
  }
  
  q.prepare()
  emit pageChanged(property("currentPage").toInt(), __max_page);
};

void PreviewModel::nextPage() {
  int cp = property("currentPage").toInt() + 1;
  setProperty("currentPage", cp);
  updateQuery();
}

void PreviewModel::prevPage() {
  int cp = property("currentPage").toInt() - 1;
  setProperty("currentPage", cp);
  updateQuery();
}

void PreviewModel::firstPage() {
  setProperty("currentPage", 1);
  updateQuery();
}

void PreviewModel::lastPage() {
  setProperty("currentPage", __max_page);
  updateQuery();
}

const int& PreviewModel::maxPage() const {
  return __max_page;
}

void PreviewModel::setLimit(int l) {
  int cl = property("rowLimit").toInt();
  if(cl != l) {
    setProperty("rowLimit", l);
    emit limitChanged(l);
    updateQuery();
  }
}

bool PreviewModel::insertRecord(int w, const QSqlRecord& rc) {
  beginInsertRows(QModelIndex(), w, w);
  for(int c=0; c<rc.count(); ++c) {
    if(!rc.value(c).isNull()) {
      setData(index(w, c), rc.value(c));
    }
  }
  endInsertRows();
  qDebug() << rc;
  emit dataChanged(index(w, 0), index(w, rc.count()), {Qt::DisplayRole, Qt::EditRole});
  return true;
}

QVariant PreviewModel::data(const QModelIndex& mi, int role) const {
  return QSqlQueryModel::data(mi, role);
}