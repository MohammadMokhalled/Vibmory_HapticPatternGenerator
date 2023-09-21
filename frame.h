#ifndef FRAME_H
#define FRAME_H

#include <QVector>
#include <QColor>
#include <QString>

class Frame
{
public:
    /**
     * Constructor for creating a Frame object
     * @param columns The number of columns in the frame
     * @param rows The number of rows in the frame
     */
    Frame(qint32 columns, qint32 rows);

    /**
     * Constructor for creating a Frame object from a CSV string
     * @param csv The CSV string representing the frame data
     * @param columns The number of columns in the frame
     * @param rows The number of rows in the frame
     */
    Frame(QString& csv, qint32 columns, qint32 rows);

    /**
     * Set the amplitude value for a specific cell in the frame
     * @param row The row index of the cell
     * @param column The column index of the cell
     * @param value The amplitude value to set
     */
    void setAmplitude(qint32 row, qint32 column, quint32 value);

    /**
     * Set the frequency value for a specific cell in the frame
     * @param row The row index of the cell
     * @param column The column index of the cell
     * @param value The frequency value to set
     */
    void setFrequency(qint32 row, qint32 column, quint32 value);

    /**
     * Get the color of a specific cell in the frame
     * @param row The row index of the cell
     * @param column The column index of the cell
     * @return The color of the cell
     */
    QColor getColor(qint32 row, qint32 column);

    /**
     * Get the amplitude value of a specific cell in the frame
     * @param row The row index of the cell
     * @param column The column index of the cell
     * @return The amplitude value of the cell
     */
    qint32 getAmplitude(qint32 row, qint32 column);

    /**
     * Get the frequency value of a specific cell in the frame
     * @param row The row index of the cell
     * @param column The column index of the cell
     * @return The frequency value of the cell
     */
    qint32 getFrequency(qint32 row, qint32 column);

    /**
     * Convert the frame data to a string representation
     * @return The string representation of the frame data
     */
    QString toString() const;

    /**
     * Check if there was an error in the frame data
     * @return True if there was an error, false otherwise
     */
    bool getError();

private:
    qint32 mRows;
    qint32 mColumns;
    bool mCreationError;
    QVector<QVector<quint32>> mFrequencies;
    QVector<QVector<quint32>> mAmplitudes;
    QVector<QVector<QColor>> mColors;

    /**
     * Sets the color for the specified row and column.
     *
     * @param row The row index.
     * @param column The column index.
     */
    void setColor(qint32 row, qint32 column);
    
    /**
     * Sets the error.
     */
    void setError();
};

#endif // FRAME_H
