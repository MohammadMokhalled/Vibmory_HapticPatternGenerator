#ifndef PROJECTSETTING_H
#define PROJECTSETTING_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QVector>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include "paintingwidget.h"
#include "animationcontroller.h"
namespace Ui {
class projectsettingwindow;
}

class ProjectSettingWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a ProjectSettingWindow object with a parent widget.
     * @param parent The parent widget (default: nullptr).
     */
    explicit ProjectSettingWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Constructs a ProjectSettingWindow object with the specified number of rows and columns, and a parent widget.
     * It could be used for creating new blank projects
     *
     * @param rows The number of rows.
     * @param columns The number of columns.
     * @param parent The parent widget (default: nullptr).
     */
    explicit ProjectSettingWindow(const QSize& size, QWidget *parent = nullptr);
    
    /**
     * @brief Constructs a ProjectSettingWindow object with the specified animation and a parent widget.
     * It is used for loading saved projects
     *
     * @param animation The Animation object.
     * @param parent The parent widget (default: nullptr).
     */
    explicit ProjectSettingWindow(const QString& fileName, QWidget *parent = nullptr);
    ~ProjectSettingWindow();

public slots:
    /**
     * Enables the group box.
     */
    void enableGroupBox();
    
    /**
     * Stops the play the animation.
     */
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
    QSize mSize;
    QGraphicsView* mGraphicViews;
    AnimationController* mAnimationController;
    quint16 mCurrentFrame;
    QPoint mCurrentCell;
    quint8 mTabChangeTries;
    bool mEnableUnselect;
    QMediaPlayer* mPlayer;
    QFile* mSourceFile;
    QAudioOutput* mAudio;

    /**
     * Save project to file
     */
    void saveToFile();

    /**
     * Import project data from file
     */
    void importFromFile();

    /**
     * Initialize the user interface
     */
    void initializeUI();

    /**
     * Initialize the slots
     */
    void initializeSlots();

    /**
     * Initialize the UI and Slots
     */
    void initialize();

    /**
     * Start playing the animation
     */
    void startPlay();
};

#endif // PROJECTSETTING_H
