#ifndef FRAME_H
#define FRAME_H

#include <QVector>
#include <QColor>
#include <QString>

class Frame
{
private:
    QVector<QVector<uint32_t>> frequencies;
    QVector<QVector<uint32_t>> amplitudes;
    QVector<QVector<QColor>> colors;
    int rows, columns;
    void setColor(int row, int column);

public:
    Frame(int columns, int rows);
    Frame(QString csv, int columns, int rows);
    void setAmplitude(int row, int column, uint32_t value);
    void setFrequency(int row, int column, uint32_t value);
    QColor getColor(int row, int column);
    int getAmplitude(int row, int column);
    int getFrequency(int row, int column);
    QString toString();

};

#endif // FRAME_H
