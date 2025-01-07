#ifndef TENTANGAPLIKASI_H
#define TENTANGAPLIKASI_H

#include <QWidget>

namespace Ui {
class TentangAplikasi;
}

class TentangAplikasi : public QWidget
{
    Q_OBJECT

public:
    explicit TentangAplikasi(QWidget *parent = nullptr);
    ~TentangAplikasi();

private:
    Ui::TentangAplikasi *ui;
};

#endif // TENTANGAPLIKASI_H
