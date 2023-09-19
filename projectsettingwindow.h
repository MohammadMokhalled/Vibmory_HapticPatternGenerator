#ifndef PROJECTSETTING_H
#define PROJECTSETTING_H

#include <QMainWindow>
#include "qgraphicsview.h"
#include "paintingwidget.h"
#include "helper.h"
#include <QVector>
#include <frame.h>
#include <animation.h>
#include <QMediaPlayer>
#include <QAudioOutput>

namespace Ui {
class projectsettingwindow;
}

class ProjectSettingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ProjectSettingWindow(QWidget *parent = nullptr);
    explicit ProjectSettingWindow(qint32 rows, qint32 columns, QWidget *parent = nullptr);
    explicit ProjectSettingWindow(Animation* animation, QWidget *parent = nullptr);
    ~ProjectSettingWindow();

public slots:
    void enableGroupBox();
    void stopPlay();

private slots:


    void addFrame();

    void on_addFrameToolButton_clicked();

    void on_playPushButton_clicked();

    void on_frequencySlider_valueChanged(qint32 value);

    void on_frequencySpinBox_valueChanged(qint32 arg1);

    void on_amplitudeSlider_valueChanged(qint32 value);

    void on_amplitudeSpinBox_valueChanged(qint32 arg1);

    void on_loopSlider_valueChanged(qint32 value);

    void on_loopSpinBox_valueChanged(qint32 arg1);

    void on_tabWidget_currentChanged(qint32 index);

    void on_previousPushButton_clicked();

    void on_nextPushButton_clicked();

    void on_saveAction_triggered(bool action);

    void on_importAction_triggered(bool action);

    void on_duplicatePushButton_clicked();

    void on_deletePushButton_clicked();

    void on_generateSounFileButton_clicked();

    void on_actionNew_Project_triggered();

    void on_maxFrequencyPushButton_clicked();

    void on_maxAmplitudePushButton_clicked();

    void on_minFrequencyPushButton_clicked();

    void on_minAmplitudePushButton_clicked();

private:
    Ui::projectsettingwindow *ui;
    qint32 mRows;
    qint32 mColumns;
    QGraphicsView* mGraphicViews;

    Helper* mHelper;
    PaintingWidget* mPaintingWidget;

    Animation* mAnimation;

    quint16 mCurrentFrame   = 0;
    quint16 mCurrentRow     = 0;
    quint16 mCurrentColumn  = 0;
    quint8 mTabChangeTries  = 0;
    bool mEnableUnselect  = 0;
    QTimer* mTimer;
    QTimer* mStopTimer;
    QMediaPlayer* mPlayer;

    QFile* mSourceFile;
    QAudioOutput* mAudio;

    void saveToFile();
    void importFromFile();
    void initializeUI();
    void initialize(Animation* animation);
    void startPlay();
};

#endif // PROJECTSETTING_H
