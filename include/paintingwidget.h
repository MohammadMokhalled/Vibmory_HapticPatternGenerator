#ifndef PAINTINGWIDGET_H
#define PAINTINGWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include "helper.h"

class PaintingWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constructor for the PaintingWidget class
     * 
     * @param helper The helper object used for painting
     * @param parent The parent widget
     */
    PaintingWidget(Helper* helper, QWidget* parent = nullptr);
    
    /**
     * Event handler for mouse press events
     * 
     * @param evt The mouse event object
     */
    void mousePressEvent(QMouseEvent* evt);
    
    /**
     * Method to draw the background
     */
    void drawBackground();

signals:
    /**
     * This function handles the cell selection.
     * 
     * @param point The coordinates of the mouse click.
     */
    void mousePressed(const QPoint& point);
    
    /**
     * This function emits a signal when an item is selected.
     */
    void selectedSignal();

public slots:
    void animate();


protected:
    /**
     * This method is called when a paint event occurs.
     * It is responsible for painting a frame.
     *
     * @param event The paint event object.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    Helper* mHelper;
    qint32 mElapsed;
    QPoint mPoint;
};

#endif // PAINTINGWIDGET_H
