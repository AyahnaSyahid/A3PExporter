#include "incl/a3database.h"
namespace A3DataQuery {

QString tableCreate =
    "CREATE TABLE IF NOT EXISTS a3pdata ( "
    "id INTEGER PRIMARY KEY, "
    "created DATETIME DEFAULT (datetime('now', 'localtime')), "
    "klien TEXT COLLATE NOCASE, "
    "file TEXT COLLATE NOCASE, "
    "bahan TEXT COLLATE NOCASE, "
    "jkertas INTEGER, "
    "jkopi INTEGER, "
    "sisi INTEGER, "
    "keterangan TEXT COLLATE NOCASE, "
    "exportName TEXT COLLATE NOCASE, "
    "modified DATETIME )";

QString tableInsert =
    "INSERT INTO a3pdata (klien, file, bahan, jkertas, jkopi, sisi, keterangan, exportName) "
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?) ";

QString tableUpdate =
    "UPDATE a3pdata SET "
    "klien = ?, "
    "file = ?, "
    "bahan = ?, "
    "jkertas = ?"
    "jkopi = ?, "
    "sisi = ?, "
    "keterangan = ?,"
    "exportName = ?, "
    "modified = datetime('now', 'localtime'), "
    "WHERE id = ?;";
}

QSqlDatabase *A3DataBase::defBase = nullptr;

A3DataBase::A3DataBase(QObject *parent) :
    QObject(parent)
{
    m_db = new QSqlDatabase;
    if(!defBase) {
        *m_db = QSqlDatabase::addDatabase("QSQLITE");
        QDir appPath(qApp->applicationDirPath());
        if(!appPath.exists("data"))
            appPath.mkdir("data");
        QString dbName = appPath.absoluteFilePath("data/a3p.db");
        setDataBaseName(dbName);
        defBase = m_db;
    }
    if(!m_db->isOpen())
        m_db->open();
    QSqlQuery n;
    bool ok = n.exec(A3DataQuery::tableCreate);
    if(!ok)
//        qDebug() << n.lastError().text();
    assert(m_db->tables().contains("a3pdata"));
    model = new A3PDataModel(this);
    model->setTable("a3pdata");
    model->setSort(1, Qt::AscendingOrder);
    model->setEditStrategy(model->OnRowChange);
    model->select();

    connect(this, &A3DataBase::dataInserted, model, &A3PDataModel::select);
    connect(this, &A3DataBase::dataInserted, model, [=](const qlonglong &_id){emit dataUpdated(_id);});
    connect(this, &A3DataBase::dataUpdated, model, &A3PDataModel::select);
    connect(this, &A3DataBase::dataRemoved, model, &A3PDataModel::select);
    connect(this, &A3DataBase::dataRemoved, model, [=](const qlonglong &_id){emit dataUpdated(_id);});
}

A3DataBase::~A3DataBase()
{
    if(m_db->isOpen())
        m_db->close();
}

const QString &A3DataBase::dataBaseName() const
{
    return m_dataBaseName;
}

void A3DataBase::setDataBaseName(const QString &newDataBaseName)
{
    if (m_dataBaseName == newDataBaseName)
        return;
    m_dataBaseName = newDataBaseName;
    m_db->setDatabaseName(newDataBaseName);
    if(!m_db->isOpen())
        m_db->open();
    QSqlQuery n;
    n.exec(A3DataQuery::tableCreate);
    emit dataBaseNameChanged();
}

A3PDataModel *A3DataBase::a3dataTable()
{
    if(!model)
    {
        model = new A3PDataModel(this);
        model->setTable("a3pdata");
        model->select();
    }
    return model;
}

void A3DataBase::insert(const QVariantMap &val)
{
    QSqlQuery p;
    p.prepare(A3DataQuery::tableInsert);
    p.bindValue(0, val["klien"]);
    p.bindValue(1, val["file"]);
    p.bindValue(2, val["bahan"]);
    p.bindValue(3, val["jkertas"]);
    p.bindValue(4, val["jkopi"]);
    p.bindValue(5, val["sisi"]);
    p.bindValue(6, val["keterangan"]);
    p.bindValue(7, val["exportName"]);
    if(p.exec())
    {
        emit dataInserted(p.lastInsertId().toLongLong());
    }
    else
    {
        emit operationFail(p.lastError().text());
    }
}

void A3DataBase::insert(const QString &str)
{
    QVariantMap m = fileStringToMap(str);
    if(!m.count())
    {
        emit operationFail("Nama file tidak valid");
        return;
    }
    return insert(m);
}

QVariantMap A3DataBase::fileStringToMap(const QString &str)
{
    QFileInfo fi(str);
    QVariantMap qm;
    /* REVISI 20220115
     *  -> gagal save dengan nama file yang
     *  mengandung '.'(titik) di pertengahan nama file
            CODE AWAL =  QStringList sl = fi.baseName().toUpper().split("_");
     */
    QStringList sl = fi.completeBaseName().toUpper().split("_");
    if(sl.count() < 4)
        return qm;
    qm.insert("klien", sl[0]);
    qm.insert("file", sl[1]);
    qm.insert("bahan", sl[2]);
    int page = 1, count;
    QStringList qty = sl[3].split("@", Qt::SkipEmptyParts);
//    qDebug() << "qty = " << qty;
    count = qty[0].toInt();
    if(qty.count() > 1)
    {
        page =  qty[0].toInt();
        count =  qty[1].toInt();
    }
//    qDebug() << page << " and " << count ;
    qm.insert("jkertas", page);
    qm.insert("jkopi", count);
    bool sided = sl.contains("BB");
    qm.insert("sisi", sided ? 2 : 1);
    QString ket;
    if(sl.count() > 4)
    {
        for(int i = 4; i < sl.count(); ++i)
        {
            ket += QString("%1 ").arg(sl[i]);
        }
    }

    qm.insert("keterangan", ket);
    qm.insert("exportName", fi.fileName());
    return qm;
}

void A3DataBase::_delete(const qlonglong &_id)
{
    QSqlQuery p;
    p.prepare("DELETE FROM a3pdata WHERE id = ?");
    p.bindValue(0, _id);
    bool ok = p.exec();
    if(ok)
        emit dataRemoved(_id);
    else
        emit operationFail(p.lastError().text());
}

void A3DataBase::_update(const qlonglong &_id, const QVariantMap &val)
{
    QSqlQuery p;
    p.prepare(A3DataQuery::tableUpdate);
    p.bindValue(0, val["klien"]);
    p.bindValue(1, val["file"]);
    p.bindValue(2, val["bahan"]);
    p.bindValue(3, val["jkertas"]);
    p.bindValue(4, val["jkopi"]);
    p.bindValue(5, val["sisi"]);
    p.bindValue(6, val["keterangan"]);
    p.bindValue(7, val["exportName"]);
    p.bindValue(8, _id);
    bool ok = p.exec();
    if(ok)
        emit dataUpdated(_id);
    else
        emit operationFail(p.lastError().text());
}

void A3DataBase::_update(const qlonglong &_id, const QString &str)
{
    QVariantMap map = fileStringToMap(str);
    if(!map.count())
    {
        emit operationFail("Nama file tidak valid");
        return;
    }
    return _update(_id, map);

}
