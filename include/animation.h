#ifndef ANIMATION_H
#define ANIMATION_H

#include "frame.h"
#include <QVector>
#include <QMutex>
#include <QtGlobal>
#include <QString>
#include <QColor>
#include <QPoint>
#include <QSize>

/**
 * @class Animation
 * Manages animation frames, allowing manipulation of individual frames.
 */
class Animation
{
public:
    /**
     * Constructs an Animation object with a specified size.
     * @param size The QSize object representing the number of rows and columns in the animation.
     */
    explicit Animation(const QSize& size);
    
    /**
     * Constructs an Animation object and loads an animation from a file.
     * @param fileAddress The address of the file to load the animation from.
     */
    explicit Animation(const QString& fileAddress);

    /**
     * Adds a new blank frame to the animation.
     */
    void addFrame();
    
    /**
     * Creates a duplicate of the current frame and adds it to the animation.
     */
    void duplicateCurrentFrame();
    
    /**
     * Selects a frame at a specific index in the animation.
     * @param index The index of the frame to be selected.
     */
    void selectFrame(int index);

    /**
     * Gets the number of frames in the animation.
     * @return The number of frames.
     */
    int getLen() const;
    
    /**
     * Sets the amplitude value at the specified position.
     * @param pos The QPoint object representing the row and column of the cell.
     * @param value The amplitude value.
     */
    void setAmplitude(const QPoint& pos, quint32 value);
    
    /**
     * Sets the frequency value at the specified position.
     * @param pos The QPoint object representing the row and column of the cell.
     * @param value The frequency value.
     */
    void setFrequency(const QPoint& pos, quint32 value);
    
    /**
     * Gets the color of a cell at a specified position.
     * @param pos The QPoint object representing the row and column of the cell.
     * @return The color at the specified position.
     */
    QColor getColor(const QPoint& pos);
    
    /**
     * Gets the amplitude of a cell at a specified position and frame index.
     * @param pos The QPoint object representing the row and column of the cell.
     * @param frameIndex The index of the frame (optional, default value is -1 which means the current frame).
     * @return The amplitude at the specified position and frame index.
     */
    int getAmplitude(const QPoint& pos, int frameIndex = -1);
    
    /**
     * Gets the frequency of a cell at a specified position and frame index.
     * @param pos The QPoint object representing the row and column of the cell.
     * @param frameIndex The index of the frame (optional, default value is -1 which means the current frame).
     * @return The frequency at the specified position and frame index.
     */
    int getFrequency(const QPoint& pos, int frameIndex = -1);

    /**
     * Writes the animation data into a file.
     * @param fileAddress The address of the file to write to.
     * @return True if the write operation is successful, false otherwise.
     */
    bool writeInFile(const QString& fileAddress);

    /**
     * Gets the size of the animation grid.
     * @return The QSize object representing the number of rows and columns.
     */
    QSize getSize() const;
    
    /**
     * Gets a string representation of the current frame.
     * @return The frame string.
     */
    QString getFrameString();
    
    /**
     * Moves to the next frame in the animation.
     */
    void nextFrame();
    
    /**
     * Gets the index of the current frame in the animation.
     * @return The current frame index.
     */
    int getCurrentFrameIndex() const;
    
    /**
     * Checks for errors in the animation object.
     * @return True if there is an error, false otherwise.
     */
    bool getError() const;
    
    /**
     * Removes the current frame from the animation.
     */
    void removeCurrentFrame();

private:
    QSize mSize;
    quint16 mCurrentFrame;
    QPoint mCurrentPos;
    bool mCreationError; 

    QVector<Frame> mFrames; 
    mutable QMutex mLock; 

    /**
     * Sets the error state of the animation object.
     */
    void setError();
};

#endif // ANIMATION_H
