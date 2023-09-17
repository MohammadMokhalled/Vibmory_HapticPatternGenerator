#ifndef FRAME_H
#define FRAME_H

#include <QVector>
#include <QColor>
#include <QString>

class Frame
{
private:
    int mRows;
    int mColumns;
    QVector<QVector<uint32_t>> mFrequencies;
    QVector<QVector<uint32_t>> mAmplitudes;
    QVector<QVector<QColor>> mColors;
    bool mCreationError = false;

    void setColor(int row, int column);
    void setError();

public:
    Frame(int columns, int rows);
    Frame(QString& csv, int columns, int rows);
    void setAmplitude(int row, int column, uint32_t value);
    void setFrequency(int row, int column, uint32_t value);
    QColor getColor(int row, int column);
    int getAmplitude(int row, int column);
    int getFrequency(int row, int column);
    QString toString();
    bool getError();

};

#endif // FRAME_H
