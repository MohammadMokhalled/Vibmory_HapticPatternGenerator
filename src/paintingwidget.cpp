#include "paintingwidget.h"
#include "helper.h"

#include <QPainter>
#include <QTimer>


PaintingWidget::PaintingWidget(Helper* helper, QWidget *parent)
    : QWidget(parent), mHelper(helper)
{
    mElapsed = 0;

    setMouseTracking(true);
}

void PaintingWidget::animate()
{
    mElapsed = (mElapsed + qobject_cast<QTimer*>(sender())->interval()) % 1000;
    update();
}

void PaintingWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    mHelper->paint(&painter, event);
    painter.end();

}

void PaintingWidget::mousePressEvent( QMouseEvent * evt )
{
    mPoint = evt->pos();
    mHelper->selectCell(mPoint);
    emit mousePressed(mPoint);
    emit selectedSignal();
}

void PaintingWidget::drawBackground()
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    mHelper->drawBackground(&painter);
}
