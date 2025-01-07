#ifndef SAVETOEXCELFILE_H
#define SAVETOEXCELFILE_H

#include <QString>

class QTableView;

bool saveToExcelFile(QTableView *view, const QString &fname = QString());

#endif // SAVETOEXCELFILE_H
