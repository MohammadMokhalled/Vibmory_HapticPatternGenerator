#ifndef FRAME_H
#define FRAME_H

#include <QVector>
#include <QColor>
#include <QString>
#include <QSize>
#include <QPoint>

class Frame
{
public:
    /**
     * Constructor for creating a Frame object
     * @param size A QSize object containing the number of columns (width) and rows (height) in the frame.
     */
    Frame(const QSize& size);
    
    /**
     * Constructor for creating a Frame object from a CSV string.
     * @param csv The CSV string representing the frame data.
     * @param size A QSize object containing the number of columns (width) and rows (height) in the frame.
     */
    Frame(const QString& csv, const QSize& size);
    
    /**
     * Set the amplitude value for a specific cell in the frame.
     * @param pos A QPoint object containing the row (y-coordinate) and column (x-coordinate) indices of the cell.
     * @param value The amplitude value to set.
     */
    void setAmplitude(const QPoint& pos, quint32 value);
    
    /**
     * Set the frequency value for a specific cell in the frame.
     * @param pos A QPoint object containing the row (y-coordinate) and column (x-coordinate) indices of the cell.
     * @param value The frequency value to set.
     */
    void setFrequency(const QPoint& pos, quint32 value);
    
    /**
     * Get the color of a specific cell in the frame.
     * @param pos A QPoint object containing the row (y-coordinate) and column (x-coordinate) indices of the cell.
     * @return The color of the cell.
     */
    QColor getColor(const QPoint& pos) const;
    
    /**
     * Get the amplitude value of a specific cell in the frame.
     * @param pos A QPoint object containing the row (y-coordinate) and column (x-coordinate) indices of the cell.
     * @return The amplitude value of the cell.
     */
    qint32 getAmplitude(const QPoint& pos) const;
    
    /**
     * Get the frequency value of a specific cell in the frame.
     * @param pos A QPoint object containing the row (y-coordinate) and column (x-coordinate) indices of the cell.
     * @return The frequency value of the cell.
     */
    qint32 getFrequency(const QPoint& pos) const;
    
    /**
     * Convert the frame data to a string representation.
     * @return The string representation of the frame data.
     */
    QString toString() const;
    
    /**
     * Check if there was an error in the frame data.
     * @return True if there was an error, false otherwise.
     */
    bool getError() const;

private:
    QSize mSize;
    bool mCreationError;
    QVector<QVector<quint32>> mFrequencies;
    QVector<QVector<quint32>> mAmplitudes;
    QVector<QVector<QColor>> mColors;
    
    /**
     * Sets the color for the specified position.
     * @param pos A QPoint object containing the row (y-coordinate) and column (x-coordinate) indices.
     */
    void setColor(const QPoint& pos);
    
    /**
     * Sets the error.
     */
    void setError();
    
    /**
     * Checks whether the given position is valid in the frame.
     * @param pos A QPoint object containing the row (y-coordinate) and column (x-coordinate) indices.
     * @return True if the position is valid, false otherwise.
     */
    bool isValidIndex(const QPoint& pos) const;
};

#endif // FRAME_H
