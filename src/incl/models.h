#ifndef MODELS_H
#define MODELS_H

#include <QObject>
#include <QSortFilterProxyModel>
#include <QSqlTableModel>

class A3PDataModel: public QSqlTableModel
{
    Q_OBJECT
    int m_currentPage;
    int m_maxRow;
    QString lastFilter;
    mutable qlonglong lastRowCount;

public:
    enum DisplayMode {CurrentDay, ShowAll};
    static int m_instance_count;
    A3PDataModel(QObject *parent = nullptr);
    qlonglong tableRowCount() const;
    int currentPage() const;
    int maxPage() const;
    const QString getSelectStatment() const;
    void setDisplayMode(DisplayMode disp);
    virtual void setFilter(const QString &filter  = QString()) override;

public slots:
    void nextPage();
    void prevPage();
    void lastPage();
    void firstPage();
    void setMaxRow(int count);
    bool hasNextPage();
    bool hasPrevPage();
    void toggleShowMode();

signals:
    void currentPageChanged();
    void maxRowChanged();
    void filterChanged();

private:
    QString limit() const;
    DisplayMode displayMode;

protected:
    virtual QString selectStatement() const override;

};


#endif // MODELS_H


