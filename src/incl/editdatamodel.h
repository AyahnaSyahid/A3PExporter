#ifndef EditDataModelH
#define EditDataModelH

#include <QSqlQueryModel>

class EditDataModel : public QSqlQueryModel
{
  Q_OBJECT
  public:
    explicit EditDataModel(QObject* parent=nullptr) {}
    ~EditDataModel() {}
    Qt::ItemFlags flags(const QModelIndex& mi) const override;

  public slots:
    virtual void setFilter(const QString& s) = 0;
    bool syncronize();
};

#endif