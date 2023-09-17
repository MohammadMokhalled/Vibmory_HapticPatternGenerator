#include "paintingwidget.h"
#include "helper.h"

#include <QPainter>
#include <QTimer>

//! [0]
PaintingWidget::PaintingWidget(Helper* helper, QWidget *parent)
    : QWidget(parent), mHelper(helper)
{
    mElapsed = 0;
    //qDebug() << QString("started");
setMouseTracking(true);
    //setFixedSize(200, 200);
}
//! [0]

//! [1]
void PaintingWidget::animate()
{
    mElapsed = (mElapsed + qobject_cast<QTimer*>(sender())->interval()) % 1000;
    update();
}
//! [1]

//! [2]
void PaintingWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    mHelper->paint(&painter, event);
    painter.end();

}
//! [2]
//!
void PaintingWidget::mousePressEvent( QMouseEvent * evt )
{
    mPoint = evt->pos();
    mHelper->selectCell(mPoint.x(), mPoint.y());
    emit mousePressed( mPoint );
    emit selectedSignal();
}



void PaintingWidget::drawBackground()
{
    const QRect re = QRect(0,0,width(), height());
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    mHelper->drawBackground(&painter);
}
