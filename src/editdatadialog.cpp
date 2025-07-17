#include "incl/editdatadialog.h"
#include "ui/ui_editdatadialog.h"
#include <QSortFilterProxyModel>

EditDataDialog::EditDataDialog(EditMode em, QWidget* parent)
  : mode(em), ui(new Ui::EditDataDialog), QDialog(parent)
{
  ui->setupUi(this);
  setWindowTitle(mode == Konsumen ? "Ubah nama Konsumen" : "Ubah nama Bahan");
  auto model = new EditDataQueryModel(this);
  auto proxy = new QSortFilterProxyModel(this);
  proxy->setSourceModel(model);
  if(mode == Konsumen) {
    model->setQuery("SELECT klien AS Nama, COUNT(*) AS Jumlah FROM a3pdata GROUP BY klien ORDER BY COUNT(*) ASC;");
    QSqlQuery q("SELECT COUNT(DISTINCT klien) FROM a3pdata");
    q.next();
    ui->label->setText(QLocale().toString(q.value(0).toInt()) + " Data");
  } else {
    model->setQuery("SELECT bahan AS Nama, COUNT(*) AS Jumlah FROM a3pdata GROUP BY bahan ORDER BY COUNT(*) ASC;");
    QSqlQuery q("SELECT COUNT(DISTINCT bahan) FROM a3pdata");
    q.next();
    ui->label->setText(QLocale().toString(q.value(0).toInt()) + " Data");
  }
  model->setProperty("dataMode", mode);
  ui->tableView->setItemDelegate(new EditDataDelegate(this));
  proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
  proxy->setFilterKeyColumn(0);
  connect(ui->filterEdit, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);
  while(model->canFetchMore())
    model->fetchMore();
  ui->tableView->setModel(proxy);
}

EditDataDialog::~EditDataDialog(){
  delete ui;
}

void EditDataDialog::on_tutupButton_clicked(){
  this->accept();
}

QVariant EditDataQueryModel::data(const QModelIndex& mi, int role) const
{
  if(!mi.isValid()) return QVariant();
  if(mi.column() == 1) {
    if(role == Qt::DisplayRole) {
      return QLocale().toString(QSqlQueryModel::data(mi, role).toInt());
    } else if(role == Qt::TextAlignmentRole) {
      return (int) (Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QSqlQueryModel::data(mi, role);
}

Qt::ItemFlags EditDataQueryModel::flags(const QModelIndex& mi) const 
{
  if(mi.column() == 0)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
  return QSqlQueryModel::flags(mi);
}

#include <QLineEdit>

void EditDataDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& ix) const
{
  auto _xtModel = static_cast<QSortFilterProxyModel*>(model);
  auto _tModel = static_cast<EditDataQueryModel*>(_xtModel->sourceModel());
  auto _tEditor = qobject_cast<QLineEdit*>(editor);
  qDebug() << "_tModel is" << _tModel ;
  qDebug() << "_tEditor is" << _tEditor ;
  qDebug() << "setting model data";
  if(_tEditor->text() == ix.data().toString())
    return;
  QSqlQuery q;
  if(_tModel->property("dataMode").value<EditDataDialog::EditMode>() == EditDataDialog::Konsumen) {
    q.prepare("UPDATE a3pdata SET klien = ? WHERE klien = ?");
    q.addBindValue(_tEditor->text());
    q.addBindValue(ix.data().toString());
  } else {
    q.prepare("UPDATE a3pdata SET bahan = ? WHERE bahan = ?");
    q.addBindValue(_tEditor->text());
    q.addBindValue(ix.data().toString());
  }
  if(!q.exec()) {
    ;;
  };
  auto qm = _tModel->query();
  qm.exec();
  _tModel->setQuery(qm);
  while(_tModel->canFetchMore())
    _tModel->fetchMore();
}