#include "paintingwidget.h"
#include "helper.h"

#include <QPainter>
#include <QTimer>

//! [0]
PaintingWidget::PaintingWidget(Helper *helper, QWidget *parent)
    : QWidget(parent), helper(helper)
{
    elapsed = 0;
    qDebug() << "started";
setMouseTracking(true);
    //setFixedSize(200, 200);
}
//! [0]

//! [1]
void PaintingWidget::animate()
{
    elapsed = (elapsed + qobject_cast<QTimer*>(sender())->interval()) % 1000;
    update();
}
//! [1]

//! [2]
void PaintingWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    helper->paint(&painter, event);
    painter.end();

}
//! [2]
//!
void PaintingWidget::mousePressEvent( QMouseEvent * evt )
{
    mPoint = evt->pos();
    helper->selectCell(mPoint.x(), mPoint.y());
    emit mousePressed( mPoint );
    emit selectedSignal();
}



void PaintingWidget::drawBackground()
{
    const QRect re = QRect(0,0,this->width(), this->height());
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    helper->drawBackground(&painter);
}
