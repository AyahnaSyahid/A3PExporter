#include "incl/savetoexcelfile.h"

#include "xlsxworkbook.h"
#include "xlsxdocument.h"
#include "xlsxformat.h"

#include <QFileDialog>
#include <QString>
#include <QTableView>
#include <QMessageBox>
#include <QAbstractItemModel>

QVariantList getRowData(QAbstractItemModel *mod, int r)
{
    QVariantList vl;
    QModelIndex t = mod->index(r, 0);
    vl << r + 1;
    vl << t.siblingAtColumn(2).data();
    vl << t.siblingAtColumn(3).data();
    vl << t.siblingAtColumn(4).data();
    vl << t.siblingAtColumn(8).data();
    vl << t.siblingAtColumn(5).data();
    vl << t.siblingAtColumn(6).data();
    vl << t.siblingAtColumn(7).data();
    vl << (t.siblingAtColumn(5).data().toInt() * t.siblingAtColumn(6).data().toInt() * t.siblingAtColumn(7).data().toInt());
    return vl;
}

bool saveToExcelFile(QTableView *view, const QString &fname)
{
    QString currentDateStr = QDateTime::currentDateTime().toString("dddd, dd MMMM yyyy");
    QXlsx::Document *document = new QXlsx::Document();
    auto _lap = document;
    auto prepMod = view->model();
    int rowCount = prepMod->rowCount(), claprow = 1;
    QXlsx::Format titleFmt, hdrFmt, tglFmt;
    auto writeRow = [&_lap](int r, QVariantList va, QList<QXlsx::Format> fmt = {})
    {
        int col = 0, vacount = va.size();
        for(; col < vacount; col++)
        {
            if(fmt.size() == vacount)
                _lap->write(r, col + 1, va.value(col), fmt.at(col));
            else if(fmt.size() > 0)
                _lap->write(r, col + 1, va.value(col), fmt.at(0));
            else
                _lap->write(r, col + 1, va.value(col));
        }
    };
    titleFmt.setHorizontalAlignment(titleFmt.AlignHCenter);
    titleFmt.setVerticalAlignment(titleFmt.AlignVCenter);
    titleFmt.setFontSize(22);
    QString dfn = titleFmt.fontName();
    titleFmt.setFontName("Times New Roman");
    titleFmt.setPatternBackgroundColor(QColor(255,255,160));
    titleFmt.setFontBold(true);

    QString namaPerc = "A3P Exporter";
    _lap->write(claprow, 1, QString("LAPORAN A3 PLUS %1").arg(namaPerc.toUpper()));
    claprow++;
    _lap->mergeCells("A1:I1", titleFmt);
    titleFmt.setFontName(dfn);
    titleFmt.setHorizontalAlignment(titleFmt.AlignRight);
    titleFmt.setFontSize(11);
    titleFmt.setPatternBackgroundColor(Qt::black);
    titleFmt.setFontColor(Qt::white);
    titleFmt.setFontItalic(true);
    _lap->write(claprow, 7, currentDateStr, titleFmt);
    claprow++;
    _lap->mergeCells("G2:I2");

    hdrFmt.setFontBold(true);
    hdrFmt.setPatternBackgroundColor(Qt::lightGray);
    hdrFmt.setVerticalAlignment(hdrFmt.AlignVCenter);
    hdrFmt.setHorizontalAlignment(hdrFmt.AlignHCenter);
    hdrFmt.setBorderStyle(hdrFmt.BorderThin);
    hdrFmt.setBorderColor(Qt::black);
    writeRow(claprow,
             QVariantList() << "No" << "Konsumen" << "File" << "Bahan" << "Keterangan" << "Halaman" << "Banyak" << "Sisi" << "Total",
             {hdrFmt});
    claprow++;

    QXlsx::Format oddRow, evtrow;
    evtrow.setBorderStyle(evtrow.BorderThin);
    evtrow.setBorderColor(Qt::black);
    oddRow.setFontBold(true);
    oddRow.setPatternBackgroundColor(QColor(0xff, 0xdd, 0xdd));
    oddRow.setBorderStyle(oddRow.BorderThin);
    for(int rowData = 0; rowData < rowCount; ++rowData)
    {
        if(getRowData(prepMod, rowData).at(1).isNull())
            continue;
        if(rowData % 2)
            writeRow(claprow, getRowData(prepMod, rowData), {oddRow});
        else
            writeRow(claprow, getRowData(prepMod, rowData), {evtrow});
        claprow++;
    }
    // fix width column

    _lap->setColumnWidth(1, 5.0);
    _lap->setColumnWidth(2, 20.0);
    _lap->setColumnWidth(3, 50.0);
    _lap->setColumnWidth(4, 14.0);
    _lap->setColumnWidth(5, 16.0);
    _lap->setColumnWidth(6, 10.0);
    _lap->setColumnWidth(7, 10.0);
    _lap->setColumnWidth(8, 10.0);
    _lap->setColumnWidth(9, 12.0);

    // fix function
    for(int n = 4; n < claprow; n++)
    {
        _lap->write(n, 9, QString("=F%1*G%1*H%1").arg(n));
    }
    // Total


    _lap->write(claprow, 7, "Jumlah", titleFmt);
    _lap->mergeCells(QString("G%1:H%1").arg(claprow));
    _lap->write(claprow, 9, QString("=SUM(I4:I%1)").arg(claprow-1), titleFmt);
    QString saveTo = QFileDialog::getSaveFileName(qobject_cast<QWidget*>(view->parent()),
                                                  "Simpan Sebagai",fname.isEmpty() ? "Laporan.xlsx" : fname, "Excel File (*.xlsx)"
                                                  );
    if(saveTo.isEmpty())
    {
        return false;
    }

    _lap->renameSheet("Sheet1", currentDateStr.remove(0, currentDateStr.indexOf(' ')));
    if(!_lap->saveAs(saveTo))
    {
        QMessageBox::warning(qobject_cast<QWidget*>(view->parent()), "Gagal Menyimpan Berkas", "Silahkan coba lagi dalam beberapa tahun kedepan");
        return false;
    }
    return true;
}
