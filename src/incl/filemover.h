#ifndef FileMover_H
#define FileMover_H

#include <QThread>
#include <QMap>
#include <QVariant>

class FileMover : public QThread
{
    Q_OBJECT
    QVariantMap params;

public:
    FileMover(const QVariantMap& mv);
    void run();

signals:
    void failed(const QVariantMap& m);
};

#endif