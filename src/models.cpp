#include "incl/models.h"
#include <qmath.h>
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>
#include <QtDebug>

int A3PDataModel::m_instance_count = 0;

A3PDataModel::A3PDataModel(QObject *parent):QSqlTableModel(parent)
{
    m_currentPage = 0;
    m_maxRow = 200;
    displayMode = CurrentDay;
    ++m_instance_count;
    setTable("a3pdata");
    connect(this, &A3PDataModel::currentPageChanged, this, &A3PDataModel::select);
}

qlonglong A3PDataModel::tableRowCount() const
{
    QSqlQuery q;
    QString fil = QString(" WHERE %1").arg(filter()), query = QString("SELECT COUNT(*) FROM %1").arg(tableName());
    if(!filter().isEmpty())
        query += fil;
    q.exec(query);
    qlonglong res = q.next() ? q.value(0).toLongLong() : 0;
    return res;
}

int A3PDataModel::currentPage() const
{
    return m_currentPage;
}

const QString A3PDataModel::getSelectStatment() const
{
    return selectStatement();
}

void A3PDataModel::setDisplayMode(DisplayMode disp)
{
    if(displayMode != disp)
        displayMode = disp;
    select();
}

void A3PDataModel::setFilter(const QString &filter_)
{
    QString curDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString dFilt = QString("DATE(created) = \"%1\"").arg(curDate);
    QStringList lfilter;
    if(!filter_.isEmpty())
        lfilter << filter_;
    if(displayMode == CurrentDay)
    {
        lfilter << dFilt;
        QSqlTableModel::setFilter(lfilter.count() > 1 ? lfilter.join(" and ") : dFilt);
    }
    else
    {
        QSqlTableModel::setFilter(filter_);
    }
    if(m_currentPage > maxPage()) {
        m_currentPage = maxPage();
    }
    
    emit filterChanged();
}

void A3PDataModel::nextPage()
{
    m_currentPage++;
    emit currentPageChanged();
}

void A3PDataModel::prevPage()
{
    m_currentPage--;
    emit currentPageChanged();
}

void A3PDataModel::lastPage()
{
    m_currentPage = maxPage() - 1;
    emit currentPageChanged();
}

void A3PDataModel::firstPage()
{
    m_currentPage = 0;
    emit currentPageChanged();
}

int A3PDataModel::maxPage() const
{
    qlonglong tc = tableRowCount();
    int mx = qCeil((double) tc / m_maxRow);
    return mx ? mx : 1 ;
}

void A3PDataModel::setMaxRow(int count)
{
    if(m_maxRow == count)
        return ;
    m_maxRow = count;
    select();
}

bool A3PDataModel::hasNextPage()
{
    return m_currentPage < (maxPage() - 1);
}

bool A3PDataModel::hasPrevPage()
{
    return m_currentPage > 0;
}

void A3PDataModel::toggleShowMode()
{
    displayMode = displayMode == ShowAll ? CurrentDay : ShowAll;
    setFilter("");
}

QString A3PDataModel::limit() const
{
    QString tpl(" LIMIT %1 OFFSET %2");
    return tpl.arg(m_maxRow).arg(m_currentPage *  m_maxRow);
}

QString A3PDataModel::selectStatement() const
{
    QString select = QSqlTableModel::selectStatement();
    if(select.endsWith(";"))
        return select.chopped(1).simplified() + limit();
    return  select + limit();
}

