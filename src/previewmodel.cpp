#include "incl/previewmodel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QtMath>
#include <QtDebug>

PreviewModel::PreviewModel(QObject* parent)
    : QSqlQueryModel(parent) {
  setProperty("currentPage", 1);
  setProperty("filterDate", "all");
  setProperty("rowLimit", 100);
  setProperty("filterBy", "all");
  setProperty("filterValue", "%");
  
  QString baseQuery(R"--(
    SELECT * FROM a3pdata WHERE 
      date(created) LIKE :filterDate AND 
      klien LIKE :filterKlien AND
      file LIKE :filterFile AND
      bahan LIKE :filterBahan AND
      CAST(jkertas AS TEXT) LIKE :filterPage AND
      CAST(jkopi AS TEXT) LIKE :filterCopy AND
      keterangan LIKE :filterInfo AND
      exportName LIKE :filterExportName 
    )--"),
  limitingQuery(R"--(
    SELECT * FROM a3pdata WHERE 
      date(created) LIKE :filterDate AND 
      klien LIKE :filterKlien AND
      file LIKE :filterFile AND
      bahan LIKE :filterBahan AND
      CAST(jkertas AS TEXT) LIKE :filterPage AND
      CAST(jkopi AS TEXT) LIKE :filterCopy AND
      keterangan LIKE :filterInfo AND
      exportName LIKE :filterExportName 
      LIMIT :limitCount OFFSET :limitOffset
    )--"),
  calculateQuery(R"--(
    SELECT COUNT(id) FROM a3pdata WHERE
      date(created) LIKE :filterDate AND 
      klien LIKE :filterKlien AND
      file LIKE :filterFile AND
      bahan LIKE :filterBahan AND
      CAST(jkertas AS TEXT) LIKE :filterPage AND
      CAST(jkopi AS TEXT) LIKE :filterCopy AND
      keterangan LIKE :filterInfo AND
      exportName LIKE :filterExportName 
  )--"),
  selectiveQuery(R"--(
    SELECT * FROM a3pdata WHERE %1 LIKE %2;
  )--");
  setProperty("baseQuery", baseQuery);
  setProperty("limitingQuery", limitingQuery);
  setProperty("calculateQuery", calculateQuery);
  updateQuery();
};

PreviewModel::~PreviewModel() {}

void PreviewModel::updateQuery() {
  QSqlQuery q;
  // QSqlQuery q, c;
  // c.prepare(property("calculateQuery").toString());
  
  
  if(property("rowLimit").toInt() > 0) {
    q.prepare(property("limitingQuery").toString());
    q.bindValue(":limitCount", property("rowLimit").toInt());
    q.bindValue(":limitOffset", (property("currentPage").toInt() - 1) * property("rowLimit").toInt());
  } else {
    q.prepare(property("baseQuery").toString());
  }
  
  q.bindValue(":filterDate", "%");
  q.bindValue(":filterKlien", "%");
  q.bindValue(":filterFile", "%");
  q.bindValue(":filterBahan", "%");
  q.bindValue(":filterPage", "%");
  q.bindValue(":filterCopy", "%");
  q.bindValue(":filterInfo", "%");
  q.bindValue(":filterExportName", "%");

  if(property("filterDate").toString() != "all") {
    q.bindValue(":filterDate", property("filterDate").toString());    
  }
  
  if(property("filterBy").toString() != "all" && property("filterValue").toString() != "%") {
    // pastikan nilai dari filterBy mengikuti aturan nama placeholder query
    q.bindValue(QString(":%1").arg(property("filterBy").toString()), property("filterValue"));
  }

  q.exec();
  setQuery(q);
  if(lastError().isValid()) {
    qDebug() << lastError();
    qDebug() << query().lastQuery();
    qDebug() << query().boundValues();
  }
  emit pageChanged(property("currentPage").toInt(), maxPage());
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
  setProperty("currentPage", maxPage());
  updateQuery();
}

int PreviewModel::maxPage() const {
  QSqlQuery q;
  q.prepare(property("calculateQuery").toString());
  auto fval = property("filterValue").toString();

  q.bindValue(":filterDate", "%");
  q.bindValue(":filterKlien", "%");
  q.bindValue(":filterFile", "%");
  q.bindValue(":filterBahan", "%");
  q.bindValue(":filterPage", "%");
  q.bindValue(":filterCopy", "%");
  q.bindValue(":filterInfo", "%");
  q.bindValue(":filterExportName", "%");

  if(property("filterDate").toString() != "all") {
    q.bindValue(":filterDate", property("filterDate").toString());    
  }
  
  if(property("filterBy").toString() != "all" && property("filterValue").toString() != "%") {
    // pastikan nilai dari filterBy mengikuti aturan nama placeholder query
    q.bindValue(QString(":%1").arg(property("filterBy").toString()), property("filterValue"));
  } else {
    q.bindValue(":filterDate", fval);
    q.bindValue(":filterKlien", fval);
    q.bindValue(":filterFile", fval);
    q.bindValue(":filterBahan", fval);
    q.bindValue(":filterPage", fval);
    q.bindValue(":filterCopy", fval);
    q.bindValue(":filterInfo", fval);
    q.bindValue(":filterExportName", fval);
  }
  
  bool ok = q.exec() && q.next();
  if(!ok) {
    qDebug() << "Error calculating maxPage";
    qDebug() << q.lastError();
    qDebug() << q.lastQuery();
    qDebug() << q.boundValues();
    qDebug() << " <<<<< Error calculating maxPage";
  }
  int rlim = property("rowLimit").toInt();
  qDebug() << "total row :" << q.value(0).toDouble(); 
  qDebug() << "current limit :" << rlim; 
  return qCeil(q.value(0).toDouble() / (rlim < 1 ? q.value(0).toDouble() : rlim));
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