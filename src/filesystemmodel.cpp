#include "incl/filesystemmodel.h"

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if(((role == Qt::EditRole) & (index.column() == 0)))
    {
        QString pth = QDir::fromNativeSeparators(filePath(index));
        if(pth.endsWith("/"))
            pth.chop(1);
        return pth;
    }
    return QFileSystemModel::data(index, role);
}
