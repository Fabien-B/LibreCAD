#include "tc_tracerwidget.h"
#include "ui_tc_tracerwidget.h"
#include "qc_applicationwindow.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_circle.h"
#include "rs_vector.h"
#include "rs_graphicview.h"
#include <QRegularExpression>
#include <QMetaEnum>

#include <QDebug>
#include <QIODevice>


TC_TracerWidget::TC_TracerWidget(QWidget *parent, const char* name) :
    QWidget(parent),
    ui(new Ui::TC_TracerWidget), ser(nullptr), action(Action::Circle3P), stage(0), hasPreview(false), preview(nullptr)
{
    QWidget::setObjectName(name);
    ui->setupUi(this);

    ser = new QSerialPort(this);

    reg = QRegularExpression("(.) ([-+]?\\d*\\.?\\d+),([-+]?\\d*\\.?\\d+)");

    connect(ui->connect_button, &QPushButton::clicked, this, &TC_TracerWidget::openSerial);
    connect(ui->port_line_edit, &QLineEdit::returnPressed, this, &TC_TracerWidget::openSerial);

    connect(ui->rectangleButton, &QPushButton::clicked, this, [=] {
        action = Action::Rectangle;
        anchors.clear();
        stage = 0;
    });

    connect(ui->polygon_button, &QPushButton::clicked, this, [=] {
        action = Action::Polygon;
        anchors.clear();
    });

    connect(ui->circlep_button, &QPushButton::clicked, this, [=] {
        action = Action::Circle3P;
        anchors.clear();
    });




    connect(ser, &QIODevice::readyRead, ser, [=]() {
        doc = QC_ApplicationWindow::getAppWindow()->getDocument();
        if(preview == nullptr) {
            preview = new RS_Preview(doc);
        }
        QByteArray ba = ser->readAll();
        auto m = reg.match(ba);

        //qDebug() << ba;

        if (m.hasMatch()) {
            auto cap = m.capturedTexts();
            QString cmd = cap[1];
            double x = cap[2].toDouble();
            double y = cap[3].toDouble();
            auto pt = RS_Vector(x, y);

            if(cmd == "p") {
                onPressed(pt);
            } else if(cmd == "m") {
                onMoved(pt);
            } else if(cmd == "e") {
                onEnd(pt);
            }
            doc->getGraphicView()->redraw(RS2::RedrawDrawing);

        } else {
            qDebug() << "no match";
        }
    });

}



void TC_TracerWidget::onPressed(RS_Vector pos) {
    switch(action) {
    case Action::Rectangle:
    break;
    case Action::CircleCenterRadius:
    {
        switch(stage) {
        case 0:
            doc->getGraphicView()->moveRelativeZero(pos);
            stage = 1;
            break;
        case 1:
        {
            stage = 0;
            auto center = doc->getGraphicView()->getRelativeZero();
            double r = center.distanceTo(pos);
            circle_data.radius = r;
            RS_Circle* circle = new RS_Circle(doc, circle_data);
            circle->setLayerToActive();
            circle->setPenToActive();

            doc->addEntity(circle);
            deletePreview();

            // upd. undo list:
            doc->startUndoCycle();
            doc->addUndoable(circle);
            doc->endUndoCycle();
            doc->getGraphicView()->redraw();
        }
        break;
        default:
            break;
        }


    }
    break;
    case Action::Circle3P:
    {
        switch(anchors.size()) {
        case 0:
        case 1:
            doc->getGraphicView()->moveRelativeZero(pos);
            anchors.append(pos);
            doc->getGraphicView()->redraw();
            break;
        case 2:
        {
            anchors.append(pos);
            auto circle = new RS_Circle(doc, RS_CircleData());
            circle->createFrom3P(anchors[0], anchors[1], anchors[2]);
            deletePreview();
            doc->addEntity(circle);
            doc->startUndoCycle();
            doc->addUndoable(circle);
            doc->endUndoCycle();
            doc->getGraphicView()->redraw();
            anchors.clear();
        }
            break;
        default:
            break;
        }
    }
    break;
    case Action::Polygon:
    {
        doc->getGraphicView()->moveRelativeZero(pos);
        anchors.append(pos);
    }
    break;
    }
}

void TC_TracerWidget::onMoved(RS_Vector pos) {
    switch(action) {
    case Action::Rectangle:
        break;
        case Action::CircleCenterRadius:
        {
            auto center = doc->getGraphicView()->getRelativeZero();
            double r = center.distanceTo(pos);
            circle_data.center = center;
            circle_data.radius = r;
            deletePreview();
            preview->addEntity(new RS_Circle(preview, circle_data));
            drawPreview();
        }
        break;
        case Action::Circle3P:
            doc->getGraphicView()->moveRelativeZero(pos);
            if(anchors.size() == 2) {
                auto circle = new RS_Circle(preview, RS_CircleData());
                circle->createFrom3P(anchors[0], anchors[1], pos);
                deletePreview();
                preview->addEntity(circle);
                drawPreview();
            }
            doc->getGraphicView()->redraw();
            break;
        case Action::Polygon:
        {
            deletePreview();
            for(int i=1; i<anchors.size(); i++) {
                preview->addEntity(new RS_Line(preview, anchors[i-1], anchors[i]));
            }
            // line to current pos
            preview->addEntity(new RS_Line(preview, anchors.last(), pos));

            if(anchors.size() >= 2) {
                preview->addEntity(new RS_Line(preview, pos, anchors.first()));
            }
            drawPreview();
        }
        break;
    }


}

void TC_TracerWidget::onEnd(RS_Vector pos) {
    Q_UNUSED(pos)
    switch(action) {
    case Action::Rectangle:
    case Action::CircleCenterRadius:
    case Action::Circle3P:
        anchors.clear();
        deletePreview();
        doc->getGraphicView()->redraw();
    case Action::Polygon:
    {
        deletePreview();
        doc->startUndoCycle();
        for(int i=1; i<anchors.size(); i++) {
            auto line = new RS_Line(doc, anchors[i-1], anchors[i]);
            line->setLayerToActive();
            line->setPenToActive();
            doc->addEntity(line);
            doc->addUndoable(line);
        }
        if(anchors.size() >= 2) {
                    auto line = new RS_Line(doc, anchors.last(), anchors.first());
                    line->setLayerToActive();
                    line->setPenToActive();
                    doc->addEntity(line);
                    doc->addUndoable(line);
        }
        doc->endUndoCycle();
        anchors.clear();
        doc->getGraphicView()->redraw();
    }
    break;
    }
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

/**
 * Deletes the preview from the screen.
 */
void TC_TracerWidget::deletePreview() {
    if (hasPreview){
        //avoid deleting NULL or empty preview
        preview->clear();
        hasPreview=false;
    }
    auto graphicView = QC_ApplicationWindow::getAppWindow()->getGraphicView();
    if(!graphicView->isCleanUp()){
        graphicView->getOverlayContainer(RS2::ActionPreviewEntity)->clear();
    }
}



/**
 * Draws / deletes the current preview.
 */
void TC_TracerWidget::drawPreview() {
    auto graphicView = QC_ApplicationWindow::getAppWindow()->getGraphicView();
    // RVT_PORT How does offset work??        painter->setOffset(offset);
    RS_EntityContainer *container=graphicView->getOverlayContainer(RS2::ActionPreviewEntity);
    container->clear();
    container->setOwner(false); // Little hack for now so we don't delete the preview twice
    container->addEntity(preview);
    graphicView->redraw(RS2::RedrawOverlay);
    hasPreview=true;
}



TC_TracerWidget::~TC_TracerWidget()
{
    delete ui;
}
