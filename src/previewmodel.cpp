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
  QSqlQuery q;
  QSqlQuery m;
  bool use_filterDate = property("filterDate").toString() != "all",
       use_filterBy = property("filterBy").toString() != "all",
       use_limit = property("rowLimit").toInt() > 0;
  
  QString counter_query,
          model_query,
          dateFilter_query,
          columnFilter_query,
          limit_query;

  counter_query = "SELECT COUNT(id) FROM a3pdata";
  model_query = "SELECT * FROM a3pdata";
  if(property("currentPage").toInt() < 1) {
      setProperty("currentPage", 1);
  }
  if(property("filterDate").toString() != "all") {
    dateFilter_query = " DATE(created) = :filterDate ";
  }
  
  if(property("filterBy").toString() == "all") {
    columnFilter_query = " klien || file || bahan || COALESCE(keterangan, '') LIKE :filterValue ";
  } else {
    QString filterValue_string = property("filterValue").toString();
    if(filterValue_string == "%" || filterValue_string.isEmpty()) {
      columnFilter_query = "";
    } else {
      columnFilter_query = QString(" %1 LIKE :filterValue ").arg(property("filterBy").toString());
    }
  };
  // get max first
  if(!dateFilter_query.isEmpty() || !columnFilter_query.isEmpty()) {
    counter_query += " WHERE ";
    model_query += " WHERE ";
    if(!dateFilter_query.isEmpty()) {
      counter_query += dateFilter_query;
      model_query += dateFilter_query;
      if(!columnFilter_query.isEmpty()) {
        counter_query += " AND ";
        model_query += " AND ";
      }
    }
    counter_query += columnFilter_query;
    model_query += columnFilter_query;
  }
  q.prepare(counter_query);
  
  if(!dateFilter_query.isEmpty()) {
    q.bindValue(":filterDate", property("filterDate"));
  }
  if(!columnFilter_query.isEmpty()) {
    q.bindValue(":filterValue", property("filterValue"));
  }
  
  
  // if(!q.exec() && q.lastError().isValid()) {
    // qDebug() << "query calculate error";
    // qDebug() << counter_query;
    // qDebug() << q.boundValues();
  // }
  
  q.next();
  int vMaxPage = q.value(0).toInt();
  __max_page = qCeil(vMaxPage / (property("rowLimit").toDouble() < 1.0 ? vMaxPage : property("rowLimit").toDouble()));
  
  // qDebug() << "CalculateQuery :\n" << q.lastQuery();
  // qDebug() << QString("MaxPage :\n %1 | RowCount : %2 | CurrentPage : %3").arg(__max_page).arg(vMaxPage).arg(property("currentPage").toInt());
  
  if(property("currentPage").toInt() > __max_page) {
    setProperty("currentPage", __max_page);
  }
  
  // get model query
  if(use_limit) {
    model_query += QString(" LIMIT %1 OFFSET %2").arg(property("rowLimit").toInt()).arg(property("rowLimit").toInt() * (property("currentPage").toInt() - 1));
  }
  
  m.prepare(model_query);
  if(!dateFilter_query.isEmpty()) {
    m.bindValue(":filterDate", property("filterDate"));
  }
  
  if(!columnFilter_query.isEmpty()) {
    m.bindValue(":filterValue", property("filterValue"));
  }
  
  // if(!m.exec() && m.lastError().isValid()) {
    // qDebug() << "query model error";
    // qDebug() << model_query;
    // qDebug() << m.boundValues();
  // }
  
  // qDebug() << "ModelQuery :" << m.lastQuery();
  setQuery(m);
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
  return false;
}

QVariant PreviewModel::data(const QModelIndex& mi, int role) const {
  return QSqlQueryModel::data(mi, role);
}