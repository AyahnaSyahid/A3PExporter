#include "incl/filemover.h"
#include <QFileInfo>
#include <QFile>
#include <QDir>

FileMover::FileMover(const QVariantMap& p) : params(p) {}

void FileMover::run() {
    auto p = params;
    QFile f(p["tempFile"].toString());
    bool ok = f.rename(p["target"].toString());
    if(!ok) {
        emit failed(p);
    }
}
