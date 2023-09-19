#ifndef ANIMATION_H
#define ANIMATION_H

#include "frame.h"
#include <QVector>
#include <QMutex>
#include <QtGlobal>

class QString;
class QColor;

class Animation
{
public:
    /**
     * Animation class constructor that takes the number of rows and columns as arguments
     * @param rows The number of rows in the animation
     * @param columns The number of columns in the animation
     */
    explicit Animation(int rows, int columns);
    
    /**
     * Animation class constructor that takes a csv file address as a QString argument
     * @param fileAddress The address of the file used for the animation
     */
    explicit Animation(const QString& fileAddress);
    
    /**
     * Function to add a frame to the animation
     */
    void addFrame();
    
    /**
     * Function to duplicate the current frame of the animation
     */
    void duplicateCurrentFrame();
    
    /**
     * Function to select a frame at a specific index in the animation
     * @param index The index of the frame to be selected
     */
    void selectFrame(int index);
    
    /**
     * Function to get the number of frames in the animation
     * @return The length of the animation
     */
    int getLen() const;
    /**
     * Animation class constructor that takes the number of rows and columns as arguments
     * @param rows The number of rows in the animation
     * @param columns The number of columns in the animation
     */
    explicit Animation(int rows, int columns);
    
    /**
     * Animation class constructor that takes the file address as a QString argument
     * @param fileAddress The address of the file used for the animation
     */
    explicit Animation(const QString& fileAddress);
    
    /**
     * Function to add a frame to the animation
     */
    void addFrame();
    
    /**
     * Function to duplicate the current frame of the animation
     */
    void duplicateCurrentFrame();
    
    /**
     * Function to select a frame at a specific index in the animation
     * @param index The index of the frame to be selected
     */
    void selectFrame(int index);
    
    /**
     * Function to get the number of frames in the animation
     * @return The length of the animation
     */
    int getLen() const;

private:
    quint16 mRows;
    quint16 mColumns;
    quint16 mCurrentFrame;
    quint16 mCurrentRow;
    quint16 mCurrentColumn;
    bool mCreationError;

    QVector<Frame> mFrames;
    mutable QMutex mLock;

    /**
     * Function to set the error state.
     */
    void setError();
};

#endif // ANIMATION_H
