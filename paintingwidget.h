#ifndef PAINTINGWIDGET_H
#define PAINTINGWIDGET_H


#include <QWidget>
#include <QMouseEvent>



//! [0]
class Helper;

class PaintingWidget : public QWidget
{
    Q_OBJECT
signals:
    void mousePressed( const QPoint& );
    void selectedSignal();

public:
    PaintingWidget(Helper *helper, QWidget *parent);
    void mousePressEvent( QMouseEvent * evt );
    void drawBackground();

public slots:
    void animate();


protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Helper *helper;
    int elapsed;
    QPoint mPoint;
};
//! [0]

#endif // PAINTINGWIDGET_H
