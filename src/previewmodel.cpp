#include "incl/previewmodel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDate>
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
    // Ambil nilai properti untuk pagination dan filter
    int _cPage = property("currentPage").toInt();
    int _cLimit = property("rowLimit").toInt();
    _cPage = _cPage > 0 ? _cPage : 1; // Pastikan halaman minimal 1
    QString _cFilterDate = property("filterDate").toString();
    QString _cFilterBy = property("filterBy").toString();
    QString _cFilterValue = property("filterValue").toString();

    // Inisialisasi query dasar
    QString countQuery = "SELECT COUNT(*) FROM a3pdata"; // Untuk menghitung total baris
    QString modelQuery = "SELECT * FROM a3pdata";       // Untuk mengambil data

    // Bangun klausa WHERE berdasarkan filter
    QStringList whereClauses;
    if (_cFilterDate != "all") {
        whereClauses << "DATE(created) = :filterDate";
    }
    if (!_cFilterValue.isEmpty() && !QString(_cFilterValue).replace("%", "").isEmpty()) {
        if (_cFilterBy != "all") {
            whereClauses << QString("%1 LIKE :filterValue").arg(_cFilterBy);
        } else {
            whereClauses << "klien || file || bahan || COALESCE(keterangan, ' ') LIKE :filterValue";
        }
    }
    if (!whereClauses.isEmpty()) {
        QString whereClause = " WHERE " + whereClauses.join(" AND ");
        countQuery += whereClause;
        modelQuery += whereClause;
    }

    // Tambahkan LIMIT dan OFFSET untuk pagination
    if (_cLimit > 0) {
        modelQuery += " ORDER BY id DESC";
        modelQuery += " LIMIT :limit OFFSET :offset";
    }

    // Eksekusi query untuk menghitung total baris
    QSqlQuery countQ;
    countQ.prepare(countQuery);
    if (countQuery.contains(":filterDate")) {
        countQ.bindValue(":filterDate", _cFilterDate);
    }
    if (countQuery.contains(":filterValue")) {
        countQ.bindValue(":filterValue", _cFilterValue);
    }
    if (countQ.exec() && countQ.next()) {
        int rows = countQ.value(0).toInt();
        if (_cLimit < 1) {
            setProperty("rowLimit", rows); // Atur rowLimit ke rows jika tidak valid
            return updateQuery();       // Panggil ulang fungsi
        }
        __max_page = qCeil(static_cast<double>(rows) / _cLimit);
        __max_page = __max_page < 1 ? 1 : __max_page; // Pastikan minimal 1 halaman
        if (_cPage > __max_page) {
            setProperty("currentPage", __max_page); // Koreksi halaman jika melebihi maksimum
            return updateQuery();                   // Panggil ulang fungsi
        }
    } else {
        qDebug() << "Error executing countQuery:" << countQ.lastError().text();
        return;
    }

    // Eksekusi query untuk mengambil data
    QSqlQuery modelQ;
    modelQ.prepare(modelQuery);
    if (modelQuery.contains(":filterDate")) {
        modelQ.bindValue(":filterDate", _cFilterDate);
    }
    if (modelQuery.contains(":filterValue")) {
        modelQ.bindValue(":filterValue", _cFilterValue);
    }
    if (modelQuery.contains(":limit")) {
        modelQ.bindValue(":limit", _cLimit);
        modelQ.bindValue(":offset", (_cPage - 1) * _cLimit);
    }
    if (!modelQ.exec()) {
        qDebug() << "Error executing modelQuery:" << modelQ.lastError().text();
        qDebug() << "Query:" << modelQ.lastQuery();
        return;
    }

    // Terapkan query ke model dan emit sinyal
    setQuery(modelQ);
    emit pageChanged(_cPage, __max_page);
}

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
  if(role == Qt::TextAlignmentRole) {
    switch (mi.column()) {
      case 4:
      case 5:
      case 6:
      case 7:
        return Qt::AlignCenter;
    }
  } else if(role == Qt::ToolTipRole) {
    return mi.siblingAtColumn(1).data().toString();
  }
  return QSqlQueryModel::data(mi, role);
}