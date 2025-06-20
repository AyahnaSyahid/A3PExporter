#include "incl/previewmodel.h"
#include "QMessageBox"

PreviewModel::PreviewModel(QObject* parent)
    : QSqlTableModel(parent) {
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
      jkertas LIKE :filterPage AND
      jkopi LIKE :filterCopy AND
      keterangan LIKE :filterInfo AND
      export_name LIKE :filterExportName
    )--"),
  limitingQuery(R"--(
    SELECT * FROM a3pdata WHERE 
      date(created) LIKE :filterDate AND
      klien LIKE :filterKlien AND
      file LIKE :filterFile AND
      bahan LIKE :filterBahan AND
      jkertas LIKE :filterPage AND
      jkopi LIKE :filterCopy AND
      keterangan LIKE :filterInfo AND
      export_name LIKE :filterExportName
      LIMIT :limitCount OFFSET :limitOffset
    )--"),
  calculateQuery(R"--(
    COUNT(id) FROM a3pdata WHERE
      date(created) LIKE :filterDate AND
      klien LIKE :filterKlien AND
      file LIKE :filterFile AND
      bahan LIKE :filterBahan AND
      jkertas LIKE :filterPage AND
      jkopi LIKE :filterCopy AND
      keterangan LIKE :filterInfo AND
      export_name LIKE :filterExportName
  )--");
  setProperty("baseQuery", baseQuery);
  setProperty("limitingQuery", limitingQuery);
  setProperty("calculateQuery", limitingQuery);
  updateQuery();
};

PreviewModel::~PreviewModel() {}

void PreviewModel::updateQuery() {
  QSqlQuery q, c;
  c.prepare(property("calculateQuery").toString());
  if(property("rowLimit").toInt() > 0) {
    q.prepare(property("limitingQuery").toString());
    q.bindValue(":limitCount", property("rowLimit").toInt());
    q.bindValue(":limitOffset", property("currentPage").toInt() * property("rowLimit").toInt());
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
  c.bindValue(":filterDate", "%");
  c.bindValue(":filterKlien", "%");
  c.bindValue(":filterFile", "%");
  c.bindValue(":filterBahan", "%");
  c.bindValue(":filterPage", "%");
  c.bindValue(":filterCopy", "%");
  c.bindValue(":filterInfo", "%");
  c.bindValue(":filterExportName", "%");
  
  if(property("filterBy").toString() != "all") {
    // pastikan nilai dari filterBy mengikuti aturan nama placeholder query
    q.bindValue(QString(":%1").arg(property("filterBy").toString()), property("filterValue"));
    c.bindValue(QString(":%1").arg(property("filterBy").toString()), property("filterValue"));
  }

  if(property("filterDate").toString() != "all") {
    q.bindValue(":filterDate", property("filterDate").toString());    
    c.bindValue(":filterDate", property("filterDate").toString());    
  }

  c.exec();
  q.exec();
  setQuery(q);
  if(property("rowLimit").toInt() > 0) {
    emit pageChanged(property("currentPage").toInt(), qCeil(c.value(0).toInt() / property("rowLimit").toInt()));
  } else {
    emit pageChanged(property("currentPage").toInt(), 1);
  }
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
  setProperty("currentPage", maxPage);
  updateQuery();
}

void PreviewModel::maxPage() {
  QSqlQuery q;
  q.prepare(property("calculateQuery").toString());
  q.bindValue(":filterKlien", "%");
  q.bindValue(":filterFile", "%");
  q.bindValue(":filterBahan", "%");
  q.bindValue(":filterPage", "%");
  q.bindValue(":filterCopy", "%");
  q.bindValue(":filterInfo", "%");
  q.bindValue(":filterExportName", "%");
  q.bindValue(":filterDate", "%");
  if(property("filterDate").toString() != "all") {
    q.bindValue(":filterDate", property("filterDate").toString());    
  }
  return qCeil(q.value(0).toDouble() / property("rowLimit").toInt());
}

void PreviewModel::setLimit(int l) {
  int cl = property("rowLimit").toInt();
  if(cl != l) {
    setProperty("rowLimit", l);
    emit limitChanged(l);
    updateQuery();
  }
}