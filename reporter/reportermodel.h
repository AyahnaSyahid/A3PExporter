#ifndef REPORTERMODEL_H
#define REPORTERMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

class ListFileNameModel;
class PrintPreviewModel;
class ReporterModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ReporterModel(QObject *parent = nullptr, QList<QMap<QString, QVariant>> *sd = nullptr);
    inline ListFileNameModel *getFileNameModel() {return lfn; }
    inline PrintPreviewModel *getPreviewModel() {return ppm;}
    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    bool hasModified();
    void reset();


private:
    QList<QMap<QString, QVariant>> *s_data;
    QList<QMap<QString, QVariant>> buffer;
    ListFileNameModel *lfn;
    PrintPreviewModel *ppm;
};

#endif // REPORTERMODEL_H
