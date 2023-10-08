#ifndef ANIMATION_CONTROLLER_H
#define ANIMATION_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QtGlobal>
#include <QTimer>
#include "animation.h"
#include "audioactuator.h"
#include "helper.h"
#include "paintingwidget.h"


class AnimationController : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructs an AnimationController object and loads an animation from a
     * csv file.
     * @param fileName The address of the file to load the animation from.
     */
    AnimationController(const QString& fileName, QObject* parent = nullptr);

    /**
     * Constructs an AnimationController object with a specified size.
     * @param size The QSize object representing the number of rows and columns
     * in the animation.
     */
    AnimationController(const QSize &size, QObject *parent = nullptr);

    AnimationController() = delete;

    /**
     * destructor of AnimationController.
     */
    ~AnimationController();

    /**
     * Gets the number of rows and columns in the animation.
     * @return The size of the animation.
     */
    QSize getSize() const;

    /**
     * Gets the number of frames in the animation.
     * @return The size of the animation.
     */
    int getLen() const;

    /**
     * add a new blank frame to the animation.
     */
    void addFrame();

    /**
     * add duplicate of current frame to the animation.
     */
    void duplicateCurrentFrame();

    /**
     * select a frame in the animation.
     * @param index The index of the frame to select.
     */
    void selectFrame(int index);

    /**
     * Gets the PaintingWidget object.
     * @return The PaintingWidget object pointer.
     */
    QWidget* getPaintingWidget();

    /**
     * This function is responsible for painting on the QPainter object.
     */
    void drawBackground();

    /**
     * play the animation.
     */
    void playAnimation(quint16 framerate, quint32 duration);

    /**
     * stop the animation.
     */
    void stopAnimation();

    /**
     * save the animation to a csv file.
     * @param fileName The address of the file to save the animation to.
     */
    void saveToFile(const QString& fileName);

    /**
     * set frame rate of the animation. 
     * @param frameRate The frame rate of the animation.
     */
    void setFrameRate(qint32 frameRate);

    /**
     * set duration of the animation in seconds.
     * @param duration The duration of the animation in seconds.
     */
    void setDuration(qint32 duration);

    /**
     * generate audio file of the animation.
     * @param fileName The address of the file to save the audio to.
     */
    bool prepareActuator(QString &fileName);

    /**
     * get amplitude of a pixel in the animation.
     * @param pos The position of the pixel.
     * @return The amplitude of the pixel.
     */
    int getAmplitude(const QPoint& pos = QPoint(-1,-1)) const;

    /**
     * get frequency of a cell in the animation.
     * @param pos The position of the cell, default currently selected.
     * @return The frequency of the pixel.
     */
    int getFrequency(const QPoint &pos = QPoint(-1, -1)) const;

    /**
     * set the amplitude of the currenyly selected cell in the animation.
     * @param value The amplitude of the cell.
     */
    void setAmplitude(quint32 value);

    /**
    * set the frequency of the currenyly selected cell in the animation.
    * @param value The frequency of the cell.
    */
    void setFrequency(quint32 value);

    /**
     * unselect cell.
     * @param pos The position of the cell.
     */
    void unselect();

    /**
     * remove the current frame from the animation.
     */
    void removeFrame();

    /**
     * Checks if a cell is currently selected.
     * @return true if a cell is selected, false otherwise.
     */
    bool isSelected() const;

signals:
    void selectedSignal();
    void stopAnimationSignal();

private:
    Animation *mAnimation;
    IActuatorController *mAudioActuator;
    Helper *mHelper;
    PaintingWidget *mPaintingWidget;
    QTimer *mTimer;
    QTimer *mStopTimer;
};

#endif
