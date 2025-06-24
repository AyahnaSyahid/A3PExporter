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
  updateQuery();
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
  
  if(property("filterDate").toString() != "all") {
    dateFilter_query = " DATE(created) = :filterDate ";
  }
  
  if(property("filterBy").toString() == "all") {
    columnFilter_query = "klien || file || bahan || jkertas || jkopi || keterangan LIKE :filterValue";
  } else {
    QString filterValue_string = property("filterValue").toString();
    if(filterValue_string != "%" && ! filterValue_string.isEmpty()) {
      columnFilter_query = QString("%1 LIKE :filterValue").arg(property("filterBy").toString());
    } else {
      columnFilter_query = "";
    }
  }
  
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
  
  
  if(!q.exec() && q.lastError().isValid()) {
    qDebug() << "query calculate error";
    qDebug() << counter_query;
    qDebug() << q.boundValues();
  }
  q.next();
  int vMaxPage = q.value(0).toInt();
  __max_page = qCeil(vMaxPage / (property("rowLimit").toInt() < 1 ? 1 : property("rowLimit").toInt()));
  
  qDebug() << QString("MaxPage : %1 | RowCount : %2 | CurrentPage : %3").arg(__max_page).arg(vMaxPage).arg(property("rowLimit").toInt());
  
  if(property("currentPage").toInt() > __max_page) {
    setProperty("currentPage", __max_page);
  }
  
  // get model query
  if(use_limit) {
    model_query += QString(" LIMIT %1 OFFSET %2").arg(property("rowLimit").toInt()).arg(property("rowLimit").toInt() * property("currentPage").toInt());
  }
  
  m.prepare(model_query);
  
  if(!dateFilter_query.isEmpty()) {
    m.bindValue(":filterDate", property("filterDate"));
  }
  
  if(!columnFilter_query.isEmpty()) {
    m.bindValue(":filterValue", property("filterValue"));
  }
  
  if(!m.exec() && m.lastError().isValid()) {
    qDebug() << "query model error";
    qDebug() << model_query;
    qDebug() << m.boundValues();
  }
  
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
  // QSqlQuery q;
  // q.prepare(property("calculateQuery").toString());
  // auto fval = property("filterValue").toString();

  // q.bindValue(":filterDate", "%");
  // q.bindValue(":filterKlien", "%");
  // q.bindValue(":filterFile", "%");
  // q.bindValue(":filterBahan", "%");
  // q.bindValue(":filterPage", "%");
  // q.bindValue(":filterCopy", "%");
  // q.bindValue(":filterInfo", "%");
  // q.bindValue(":filterExportName", "%");

  // if(property("filterDate").toString() != "all") {
    // q.bindValue(":filterDate", property("filterDate").toString());    
  // }
  
  // if(property("filterBy").toString() != "all" && property("filterValue").toString() != "%") {
    // pastikan nilai dari filterBy mengikuti aturan nama placeholder query
    // q.bindValue(QString(":%1").arg(property("filterBy").toString()), property("filterValue"));
  // } else {
    // q.bindValue(":filterDate", fval);
    // q.bindValue(":filterKlien", fval);
    // q.bindValue(":filterFile", fval);
    // q.bindValue(":filterBahan", fval);
    // q.bindValue(":filterPage", fval);
    // q.bindValue(":filterCopy", fval);
    // q.bindValue(":filterInfo", fval);
    // q.bindValue(":filterExportName", fval);
  // }
  
  // bool ok = q.exec() && q.next();
  // if(!ok) {
    // qDebug() << "Error calculating maxPage";
    // qDebug() << q.lastError();
    // qDebug() << q.lastQuery();
    // qDebug() << q.boundValues();
    // qDebug() << " <<<<< Error calculating maxPage";
  // }
  // int rlim = property("rowLimit").toInt();
  // qDebug() << "total row :" << q.value(0).toDouble(); 
  // qDebug() << "current limit :" << rlim; 
  // return qCeil(q.value(0).toDouble() / (rlim < 1 ? q.value(0).toDouble() : rlim));
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