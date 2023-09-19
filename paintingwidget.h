#ifndef PAINTINGWIDGET_H
#define PAINTINGWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include "helper.h"

class PaintingWidget : public QWidget
{
    Q_OBJECT

public:
    PaintingWidget(Helper* helper, QWidget* parent);
    void mousePressEvent(QMouseEvent* evt);
    void drawBackground();

signals:
    void mousePressed( const QPoint& );
    void selectedSignal();

public slots:
    void animate();


protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Helper* mHelper;
    qint32 mElapsed;
    QPoint mPoint;
};

#endif // PAINTINGWIDGET_H
