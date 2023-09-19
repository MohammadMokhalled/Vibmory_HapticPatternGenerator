#ifndef ANIMATION_H
#define ANIMATION_H

#include "frame.h"
#include <QVector>
#include <QMutex>
#include <QtGlobal>
#include <QString>
#include <QColor>

class Animation
{
public:
    /**
     * Animation constructor with specified number of rows and columns.
     * 
     * @param rows The number of rows in the animation.
     * @param columns The number of columns in the animation.
     */
    explicit Animation(int rows, int columns);

    /**
     * Animation constructor with specified csv file address.
     * 
     * @param fileAddress The address of the file to load the animation from.
     */
    explicit Animation(const QString& fileAddress);

    /**
     * Adds a new blank frame
     */
    void addFrame();
    
    /**
     * Duplicates the current frame
     */
    void duplicateCurrentFrame();
    
    /**
     * Selects a frame at a specific index in the animation
     *
     * @param index The index of the frame to be selected
     */
    void selectFrame(int index);

    /**
     * Returns number of frames in the animation.
     * 
     * @return The number of frames.
     */
    int getLen() const;
    
    /**
     * Sets the position of current cell.
     * 
     * @param row The row of the cell.
     * @param column The column of the cell.
     */
    void setPos(int row, int column);
    
    /**
     * Sets the amplitude value at the specified position.
     * 
     * @param row The row position.
     * @param column The column position.
     * @param value The amplitude value.
     */
    void setAmplitude(int row, int column, quint32 value);
    /**
     * Set the frequency value for a specific row and column.
     * 
     * @param row - The row index.
     * @param column - The column index.
     * @param value - The frequency value to set.
     */
    void setFrequency(int row, int column, quint32 value);

    /**
     * Get the color for a specific row and column.
     * 
     * @param row - The row index.
     * @param column - The column index.
     * @return The color at the specified row and column.
     */
    QColor getColor(int row, int column);

    /**
     * Get the amplitude for a specific row, column, and frame index.
     * 
     * @param row - The row index.
     * @param column - The column index.
     * @param frameIndex - The frame index (optional, default value is -1 which means the current frame).
     * @return The amplitude at the specified row, column, and frame index.
     */
    int getAmplitude(int row, int column, int frameIndex = -1);
    /**
     * Gets the frequency value of the specified row and column cell.
     *
     * @param row The row index.
     * @param column The column index.
     * @param frameIndex The index of the frame (optional, default value is -1 which means the current frame).
     * @return The frequency value at the specified row and column.
     */
    int getFrequency(int row, int column, int frameIndex = -1);

    /**
     * Writes the data into a file.
     *
     * @param fileAddress The address of the file to write to.
     * @return True if the write operation is successful, false otherwise.
     */
    bool writeInFile(const QString& fileAddress);

    /**
     * Gets the number of rows.
     *
     * @return The number of rows.
     */
    int getRows() const;
    /**
     * Get the number of columns.
     * 
     * @return int The number of columns.
     */
    int getColumns() const;

    /**
     * Get the frame string.
     * 
     * @return QString The frame string.
     */
    QString getFrameString();

    /**
     * Move to the next frame.
     */
    void nextFrame();
    /**
     * Get the current frame index.
     * 
     * @return int The current frame index.
     */
    int getCurrentFrameIndex() const;

    /**
     * Check if there is an error.
     * 
     * @return bool True if there is an error, false otherwise.
     */
    bool getError() const;

    /**
     * Remove the current frame.
     */
    void removeCurrentFrame();

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
     * Set the error state.
     */
    void setError();
};

#endif // ANIMATION_H
