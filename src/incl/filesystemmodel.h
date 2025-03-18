#ifndef FileSystemModelH
#define FileSystemModelH

#include <QFileSystemModel>

class FileSystemModel : public QFileSystemModel {
public:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

#endif