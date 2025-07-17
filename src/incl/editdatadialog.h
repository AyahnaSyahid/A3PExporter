#ifndef EditDataDialogH
#define EditDataDialogH
#include <QDialog>
#include <QtDebug>

namespace Ui {
  class EditDataDialog;
};

class EditDataDialog : public QDialog
{
  Q_OBJECT
  public:
    enum EditMode {
      Bahan, Konsumen };
      
    explicit EditDataDialog(EditMode, QWidget* =nullptr);
    ~EditDataDialog();
    const EditMode& getMode() const { return mode; }
  
  public slots:
    virtual void on_tutupButton_clicked();
  
  protected:
    Ui::EditDataDialog* ui;
  
  private:
    EditMode mode;
};

Q_DECLARE_METATYPE(EditDataDialog::EditMode)

#include <QSqlQuery>
#include <QSqlQueryModel>

class EditDataQueryModel : public QSqlQueryModel
{
  public:
    EditDataQueryModel(QObject* parent=nullptr) : QSqlQueryModel(parent) {}
    ~EditDataQueryModel() {};
    
    QVariant data(const QModelIndex&, int =Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex&) const override;
};


#include <QStyledItemDelegate>

class EditDataDelegate : public QStyledItemDelegate
{
  public:
    EditDataDelegate(QObject* parent=nullptr) : QStyledItemDelegate(parent) {}
    ~EditDataDelegate() {}
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex& mi) const override;
};

#endif