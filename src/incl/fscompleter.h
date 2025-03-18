#ifndef FSCompleterH
#define FSCompleterH

#include <QCompleter>

class FSCompleter : public QCompleter
{
public:
    QString pathFromIndex(const QModelIndex &idx) const override;
};

#endif