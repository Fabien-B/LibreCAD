#ifndef TC_TRACERWIDGET_H
#define TC_TRACERWIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QRegularExpression>
#include "rs_preview.h"
#include "rs_circle.h"

namespace Ui {
class TC_TracerWidget;
}

class TC_TracerWidget : public QWidget
{
    Q_OBJECT

public:

    enum Action {
        Rectangle,
        CircleCenterRadius,
        Circle3P,
        Polygon,
    };

    explicit TC_TracerWidget(QWidget *parent = nullptr, const char* name = 0);
    ~TC_TracerWidget();

    void drawPreview();
    void deletePreview();

private:

    void openSerial();

    void onPressed(RS_Vector pos);
    void onMoved(RS_Vector pos);
    void onEnd(RS_Vector pos);

    Ui::TC_TracerWidget *ui;

    QSerialPort* ser;

    RS_Document* doc;

    QRegularExpression reg;
    enum Action action;

    int stage;

    bool hasPreview;
    RS_Preview* preview;

    RS_CircleData circle_data;
    QList<RS_Vector> anchors;
};

#endif // TC_TRACERWIDGET_H
