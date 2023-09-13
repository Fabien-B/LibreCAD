#include "tc_tracerwidget.h"
#include "ui_tc_tracerwidget.h"
#include "qc_applicationwindow.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_circle.h"
#include "rs_vector.h"
#include "rs_graphicview.h"
#include <QRegularExpression>
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_coordinateevent.h"
#include <QMetaEnum>

#include <QDebug>
#include <QIODevice>


// TODO: see lib/gui/rs_dialogfactoryinterface.h


TC_TracerWidget::TC_TracerWidget(QWidget *parent, const char* name) :
    QWidget(parent),
    ui(new Ui::TC_TracerWidget), ser(nullptr)
{
    setObjectName(name);
    ui->setupUi(this);

    ser = new QSerialPort(this);

    reg = QRegularExpression("l (\\d+),(\\d+) (\\d+),(\\d+)");
    reg_zero = QRegularExpression("z (\\d+),(\\d+)");
    reg_circle = QRegularExpression("c (\\d+),(\\d+) (\\d+)");

    connect(ui->connect_button, &QPushButton::clicked, this, &TC_TracerWidget::openSerial);
    connect(ui->port_line_edit, &QLineEdit::returnPressed, this, &TC_TracerWidget::openSerial);


    connect(ser, &QIODevice::readyRead, ser, [=]() {
        QByteArray ba = ser->readAll() ;
        qDebug() << ba;

        auto &w = QC_ApplicationWindow::getAppWindow();
        auto doc = w->getDocument();

        auto m = reg.match(ba);
        auto m_zero = reg_zero.match(ba);
        auto m_circle = reg_circle.match(ba);

        if (m.hasMatch()) {
            auto cap = m.capturedTexts();
            qDebug() << "match !" << m.capturedTexts();

            int x0 = cap[1].toInt();
            int y0 = cap[2].toInt();
            int x1 = cap[3].toInt();
            int y1 = cap[4].toInt();




            auto start = RS_Vector(x0, y0);
            auto end = RS_Vector(x1, y1);

            auto line = new RS_Line(start, end);
            doc->addEntity(line);
            doc->getGraphicView()->redraw();

        }
        else if (m_zero.hasMatch()) {
            auto cap = m_zero.capturedTexts();
            qDebug() << "match !" << m_zero.capturedTexts();

            int x0 = cap[1].toInt();
            int y0 = cap[2].toInt();

            auto start = RS_Vector(x0, y0);

            doc->getGraphicView()->moveRelativeZero(start);
            doc->getGraphicView()->redraw();
        }
        else if (m_circle.hasMatch()) {
            auto cap = m_circle.capturedTexts();
            qDebug() << "match circle !" << m_circle.capturedTexts();

            int x0 = cap[1].toInt();
            int y0 = cap[2].toInt();
            int r =  cap[3].toInt();

            auto center = RS_Vector(x0, y0);

            doc->getGraphicView()->moveRelativeZero(center);

            auto cd = RS_CircleData(center, r);
            auto circle = new RS_Circle(doc, cd);

            doc->addEntity(circle);
            doc->getGraphicView()->redraw();
        }
        else {
            qDebug() << "no match";
        }
    });




    connect(ui->rectangleButton, &QPushButton::clicked, this, [=] {
        auto &w = QC_ApplicationWindow::getAppWindow();
        auto doc = w->getDocument();

        auto start = RS_Vector(10, 10);
        auto end = RS_Vector(30, 30);
        auto line = new RS_Line(start, end);
        doc->addEntity(line);
        doc->getGraphicView()->redraw();
    });
}

void TC_TracerWidget::openSerial() {

    if(ser->isOpen()) {
        ser->close();
        ui->connect_button->setText("connect");
    } else {
        auto port = ui->port_line_edit->text();
        ser->setPortName(port);

        bool success = ser->open(QSerialPort::ReadWrite);
        if(success) {
            ui->connect_button->setText("disconnect");
            ser->clear();
            ui->status_label->setText("Port opened.");
        }
        else {
            //RS_DIALOGFACTORY->commandMessage("QStri");
            auto err = QMetaEnum::fromType<QSerialPort::SerialPortError>().valueToKey(ser->error());
            ui->status_label->setText(QString("error opening serial: %1").arg(err));
            qDebug() << "serial port opened:" << ser->isOpen() << ser->portName() << ser->openMode() << ser->error();
        }

    }


}

TC_TracerWidget::~TC_TracerWidget()
{
    delete ui;
}
