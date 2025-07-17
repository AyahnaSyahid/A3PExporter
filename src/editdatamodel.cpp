#include "editdatamodel.h"
#include <QSqlQuery>

EditDataModel::EditDataModel(QObject* parent) 
  : QSqlQueryModel(parent)
{}

EditDataModel::~EditDataModel();

bool EditDataModel::isDirty(const QModelIndex& mi) const {
  Q_UNUSED(mi);
  return true;
}

bool EditDataModel::isDirty() const {
  return true;
}

Qt::ItemFlags flags(const QModelIndex& mi) const {
  if(mi.column() == 2) {
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
  }
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}