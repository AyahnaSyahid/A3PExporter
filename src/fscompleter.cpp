#include "incl/fscompleter.h"


QString FSCompleter::pathFromIndex(const QModelIndex &idx) const
{
    QString rv = model()->data(idx, Qt::EditRole).toString();
    return rv;
}
