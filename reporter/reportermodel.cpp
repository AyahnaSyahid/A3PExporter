#include "reportermodel.h"
#include "printpreviewmodel.h"
#include "listfilenamemodel.h"
#include "dirscanner.h"

#include <QtDebug>
#include <QColor>

ReporterModel::ReporterModel(QObject *parent, QList<QMap<QString, QVariant>> *sd)
    : lfn(new ListFileNameModel(this)), ppm(new PrintPreviewModel(this)), QAbstractTableModel(parent),
      s_data(sd)
{
    buffer = *s_data;
    lfn->setSourceModel(this);
    lfn->setFilterCaseSensitivity(Qt::CaseInsensitive);
    lfn->setFilterKeyColumn(-1);
    ppm->setSourceModel(this);
    ppm->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ppm->setFilterKeyColumn(-1);

}

int ReporterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return buffer.size();
}

int ReporterModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    if(buffer.size())
        return buffer[0].size();
    return 12;
}

QVariant ReporterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    auto buf = &buffer;
    auto keys = buf->at(index.row()).keys();
    auto value = buf->at(index.row())[keys[index.column()]];
    if(role == Qt::TextAlignmentRole)
    {
        if(QVariant(value).type() == QVariant::LongLong)
        {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        else if(QVariant(value).type() == QVariant::Int)
        {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        else {
            return QVariant();
        }
    }
    else if (role == Qt::DisplayRole)
    {
        return QVariant(value);
    }
    else if (role == Qt::EditRole)
    {
        return QVariant(value);
    }
    else if (role == Qt::ToolTipRole)
    {
        if(index.column() == 9)
            return buffer.at(index.row()).value(keys.value(11));
        return QVariant();
    }
    else if(role == Qt::ForegroundRole)
    {
        if(!data(index.siblingAtColumn(0), Qt::EditRole).toBool())
            return QVariant(QColor(200, 0, 0));
    }
    return QVariant();
}

QVariant ReporterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            if(buffer.size())
            {
                QString colname = buffer.at(0).keys().at(section);
                colname[0].toTitleCase();
                return colname;
            }
            return QVariant();
        }
        return QAbstractTableModel::headerData(section,orientation,role);
    }
    return QVariant();
}

bool ReporterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    auto buf = &buffer;
    auto keys = buf->at(index.row()).keys();
    QString keyName = keys[index.column()];
    if (data(index, role) != value) {
        buffer[index.row()][keyName] = value;
        if(keyName == "9fileName")
        {
            QString vproc = value.toString();
            vproc.remove(vproc.lastIndexOf('.'), 8);
            QStringList splitted = vproc.split('_');
            if(splitted.size() < 4)
            {
                buffer[index.row()]["0validity"] = false;
                buffer[index.row()]["1userCol"] = QVariant(QVariant::String);
                buffer[index.row()]["2fileCol"] = QVariant(QVariant::String);
                buffer[index.row()]["3matCol"] = QVariant(QVariant::String);
                buffer[index.row()]["4desCol"] = QVariant(QVariant::String);
                buffer[index.row()]["5halCol"] = QVariant(QVariant::Int);
                buffer[index.row()]["6cntCol"] = QVariant(QVariant::Int);
                buffer[index.row()]["7sideCol"] = QVariant(QVariant::Int);
                buffer[index.row()]["8sumCol"] = QVariant(QVariant::Int);
                emit dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(9));
                return true;
            }
            else
            {
                buffer[index.row()]["1userCol"] = splitted.at(0);
                buffer[index.row()]["2fileCol"] = splitted.at(1);
                buffer[index.row()]["3matCol"] = splitted.at(2);
                buffer[index.row()]["5halCol"] = 1;
                buffer[index.row()]["6cntCol"] = 1;
                buffer[index.row()]["7sideCol"] = 1;

                QString countSpek = splitted.at(3);
                if(countSpek.contains('@'))
                {
                    QStringList css = countSpek.split('@');
                    buffer[index.row()]["5halCol"] = QVariant(css[0]).toInt();
                    buffer[index.row()]["6cntCol"] = QVariant(css[1]).toInt();
                }
                else
                {
                    buffer[index.row()]["5halCol"] = 1;
                    buffer[index.row()]["6cntCol"] = splitted.at(3).toInt();
                }

                if(splitted.count() > 4)
                {
                    QStringList rest;
                    for(int i = 4; i < splitted.count(); ++i)
                    {
                        if(splitted[i].toUpper() == "BB")
                            buffer[index.row()]["7sideCol"] = 2;
                        rest << splitted[i];
                    }
                    buffer[index.row()]["4desCol"] = rest.join('-');
                }

                buffer[index.row()]["8sumCol"] = buffer[index.row()]["5halCol"].toInt() *
                                                 buffer[index.row()]["6cntCol"].toInt() *
                                                 buffer[index.row()]["7sideCol"].toInt();
                emit dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(9));
                return true;
            }
        }
        buffer[index.row()]["8sumCol"] = buffer[index.row()]["5halCol"].toInt() *
                                         buffer[index.row()]["6cntCol"].toInt() *
                                         buffer[index.row()]["7sideCol"].toInt();
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags ReporterModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool ReporterModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if(!buffer.size())
        return false;
    beginInsertRows(parent, row, row + count - 1);
    auto keys = buffer.at(0).keys();
    for(int r = row; r < row + count; r++)
    {
        QMap<QString, QVariant> temp;
        for(const QString &s : qAsConst(keys))
        {
            temp[s] = QVariant();
        }
        buffer.insert(r, temp);
    }
    endInsertRows();
    return true;
}

bool ReporterModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(!buffer.size())
        return false;
    if(buffer.size() < row)
        return false;
    beginRemoveRows(parent, row, row + count - 1);
    for(int i = row + count; i > row; --i)
        buffer.removeAt(row);
    endRemoveRows();
    return true;
}

bool ReporterModel::hasModified()
{
    if(buffer.size() != s_data->size())
        return true;
    auto bit = buffer.constBegin();
    auto sit = buffer.constBegin();

    for(;bit != buffer.constEnd(); ++bit, ++sit)
    {
        auto bbit = bit->constBegin();
        auto ssit = sit->constBegin();
        for(; bbit != bit->constEnd(); ++bbit, ++ssit)
        {
            if(bbit.value() != ssit.value())
                return true;
        }
    }
    return false;
}

void ReporterModel::reset()
{
    beginResetModel();
    buffer = *s_data;
    endResetModel();
}

ListFileNameModel::ListFileNameModel(QObject* parent)
    : QSortFilterProxyModel(parent) {}
    
ListFileNameModel::~ListFileNameModel(){}

QVariant ListFileNameModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    if(role == Qt::ForegroundRole)
    {
        if(!QSortFilterProxyModel::data(index.siblingAtColumn(0), Qt::EditRole).toBool())
            return QVariant(QColor(200, 0, 0));
        return QSortFilterProxyModel::data(index, role);
    }
    return QSortFilterProxyModel::data(index, role);
}

QVariant ListFileNameModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if((section == 9) & (orientation == Qt::Horizontal))
        if(role == Qt::DisplayRole)
            return "Nama File";
    return QSortFilterProxyModel::headerData(section, orientation, role);
}

void ListFileNameModel::setFilterWildCard(const QString &wc)
{
    if(wc.isEmpty())
    {
        invalidateFilter();
        return ;
    }
    return QSortFilterProxyModel::setFilterWildcard(wc);
}

PrintPreviewModel::PrintPreviewModel(QObject* parent) 
    : QSortFilterProxyModel(parent) {}

PrintPreviewModel::~PrintPreviewModel() {}

QVariant PrintPreviewModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    return QSortFilterProxyModel::data(index, role);
}

QVariant PrintPreviewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case 1:
                return "Konsumen";
            case 2:
                return "Nama File";
            case 3:
                return "Bahan";
            case 4:
                return "Keterangan";
            case 5:
                return "Halaman";
            case 6:
                return "Banyak";
            case 7:
                return "Sisi";
            case 8:
                return "Jumlah";
            case 9:
                return "Nama File";
            default:
                return QSortFilterProxyModel::headerData(section, orientation, role);
            }
        }
    }
    return QSortFilterProxyModel::headerData(section, orientation, role);
}

QVariantList PrintPreviewModel::rowData(const int &row) const
{
    QVariantList valist;
    QList<int> colist = {1, 2, 3, 4, 5, 6, 7, 8};
    valist.append(QVariant(row + 1));
    for(auto x = colist.constBegin(); x != colist.constEnd() ; ++x)
    {
        valist.append(data(index(row, *x), Qt::EditRole));
    }
    return valist;
}

void PrintPreviewModel::setFilterWildCard(const QString &wc)
{
    if(wc.isEmpty())
    {
        invalidateFilter();
        return ;
    }
    return QSortFilterProxyModel::setFilterWildcard(wc);
}
