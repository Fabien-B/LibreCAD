#ifndef TC_TRACERWIDGET_H
#define TC_TRACERWIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QRegularExpression>

namespace Ui {
class TC_TracerWidget;
}

class TC_TracerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TC_TracerWidget(QWidget *parent = nullptr, const char* name = 0);
    ~TC_TracerWidget();

private:

    void openSerial();

    Ui::TC_TracerWidget *ui;

    QSerialPort* ser;
    QRegularExpression reg;
    QRegularExpression reg_zero;
    QRegularExpression reg_circle;
};

#endif // TC_TRACERWIDGET_H
