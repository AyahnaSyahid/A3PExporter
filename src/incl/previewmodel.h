#ifndef PREVIEWMODEL_H
#define PREVIEWMODEL_H

#include <QSqlQueryModel>

#include <QtDebug>

class PreviewModel : public QSqlQueryModel
{
  Q_OBJECT

public:
  explicit PreviewModel(QObject *parent = nullptr);
  ~PreviewModel();
  virtual QVariant data(const QModelIndex &index, int role) const override;
  const int& maxPage() const;
  
  bool insertRecord(int, const QSqlRecord&);

public slots:
  void nextPage();
  void prevPage();
  void firstPage();
  void lastPage();

  // void filterDate(const QString& date);

  void setLimit(int lim);

  void updateQuery();

signals:
  void pageChanged(int current, int max);
  void limitChanged(int lim);

private:
  int __max_page;
};

#endif // PREVIEWMODEL_H
