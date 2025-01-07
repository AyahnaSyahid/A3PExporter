#ifndef A3DATABASE_H
#define A3DATABASE_H

#include <QObject>
#include <QVariantMap>
#include <QCoreApplication>
#include "models.h"
#include <QtSql>
#include <QtDebug>

class A3DataBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString m_dataBaseName READ dataBaseName WRITE setDataBaseName NOTIFY dataBaseNameChanged)
    QString m_dataBaseName;
    QSqlDatabase *m_db;
    A3PDataModel *model;
public:
    static QSqlDatabase *defBase;
    explicit A3DataBase(QObject *parent = nullptr);
    ~A3DataBase();
    const QString &dataBaseName() const;
    void setDataBaseName(const QString &newDataBaseName);
    A3PDataModel *a3dataTable();

public slots:
    void insert(const QVariantMap &val);
    void insert(const QString &str);
    QVariantMap fileStringToMap(const QString &str);
    void _delete(const qlonglong &_id);
    void _update(const qlonglong &_id, const QVariantMap &val);
    void _update(const qlonglong &_id, const QString &str);

signals:
    void dataInserted(const qlonglong &_id);
    void dataUpdated(const qlonglong &_id);
    void dataRemoved(const qlonglong &_id);
    void dataBaseNameChanged();
    void operationFail(const QString &msg);
};

#endif // A3DATABASE_H
