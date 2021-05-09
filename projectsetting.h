#ifndef PROJECTSETTING_H
#define PROJECTSETTING_H

#include <QMainWindow>
#include "qpushbutton.h"
#include "qgraphicsview.h"
#include "qgridlayout.h"
#include "paintingwidget.h"
#include "helper.h"
#include <QVector>
#include <frame.h>
#include <animation.h>

namespace Ui {
class projectsetting;
}

class projectsetting : public QMainWindow
{
    Q_OBJECT

public:
    explicit projectsetting(QWidget *parent = nullptr);
    explicit projectsetting(bool test, int rows, int columns, QWidget *parent = nullptr);
    explicit projectsetting(Animation * anim, QWidget *parent = nullptr);
    ~projectsetting();

    int rows;
    int columns;
    QGraphicsView *graphicViews;

    Helper * help;
    PaintingWidget * pw;

    Animation * animation;

public slots:
    void enableGroupBox();
    void stopPlay();

private slots:
    void addFrame();

    void on_addFrameToolButton_clicked();

    void on_playPushButton_clicked();

    void on_frequencySlider_valueChanged(int value);

    void on_frequencySpinBox_valueChanged(int arg1);

    void on_amplitudeSlider_valueChanged(int value);

    void on_amplitudeSpinBox_valueChanged(int arg1);

    void on_loopSlider_valueChanged(int value);

    void on_loopSpinBox_valueChanged(int arg1);

    void on_tabWidget_currentChanged(int index);

    void on_previousPushButton_clicked();

    void on_nextPushButton_clicked();

    void on_saveAction_triggered(bool action);

    void on_importAction_triggered(bool action);

    void on_duplicatePushButton_clicked();

    void on_deletePushButton_clicked();

private:
    Ui::projectsetting *ui;
    uint16_t currentFrame   = 0;
    uint16_t currentRow     = 0;
    uint16_t currentColumn  = 0;
    uint8_t tabChangeTries  = 0;
    bool enableUnselect  = 0;
    QTimer *timer;
    QTimer *stopTimer;

    void saveToFile();
    void importFromFile();
    void initializeUI();
    void initialize(Animation * anim);
    void startPlay();

};

#endif // PROJECTSETTING_H
