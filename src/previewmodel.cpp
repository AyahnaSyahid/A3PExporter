#include "incl/previewmodel.h"
#include "QMessageBox"

PreviewModel::PreviewModel(QObject *parent) : QSqlTableModel(parent)
{
    setEditStrategy(this->OnManualSubmit);
    connect(this, &PreviewModel::pageChanged, this, &PreviewModel::select);
    connect(this, &PreviewModel::limitChanged, this, &PreviewModel::select);
}

QVariant PreviewModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::TextAlignmentRole)
    {
        switch (index.column()) {
        case 0:
        case 5:
        case 6:
        case 7:
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        case 1:
        case 10:
            return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
        default:
            return QVariant();
        }
    }
    if(role == Qt::ToolTipRole)
        return data(index.siblingAtColumn(1), Qt::DisplayRole);
    return QSqlTableModel::data(index, role);
}

QString PreviewModel::limitStatement() const
{
    QString limit =  QString(" LIMIT %1 OFFSET %2").arg(rowLimit).arg(showPage * rowLimit);
    return limit;
}

void PreviewModel::setLimit(const int &lim)
{
    if(lim == rowLimit)
        return ;
    rowLimit = lim;
    emit limitChanged();
}

void PreviewModel::setLimit(const QString &lim)
{
    bool ok;
    int nlim = lim.toInt(&ok);
    if(ok)
        setLimit(nlim);
}

void PreviewModel::setDisplayPage(const QString &page)
{
    bool okConv;
    int rpage = page.toInt(&okConv);
    if(okConv && rpage != showPage)
    {
        setDisplayPage(rpage);
    }
}

void PreviewModel::setDisplayPage(const int &page)
{
    if(page != showPage)
    {
        showPage = page;
        emit pageChanged();
    }
}

QString PreviewModel::selectStatement() const
{
    QString ret = QSqlTableModel::selectStatement() + limitStatement();
    // qDebug() << ret;
    return ret;
}

void PreviewModel::sort(int column, Qt::SortOrder ord)
{
    if(isDirty())
    {
        QMessageBox::StandardButton bt = QMessageBox::question(nullptr,
                                                               "Konfirmasi",
                                                               "Perubahan data pada table yang belum disimpan akan hilang,\n"
                                                               "abaikan ?", QMessageBox::StandardButton::Yes |
                                                               QMessageBox::StandardButton::No, QMessageBox::StandardButton::No);
        if(bt == QMessageBox::StandardButton::Yes)
            return QSqlTableModel::sort(column, ord);
        else if(bt == QMessageBox::No)
            return;
    }
    return QSqlTableModel::sort(column, ord);
}

