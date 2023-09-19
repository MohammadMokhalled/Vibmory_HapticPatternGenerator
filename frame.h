#ifndef FRAME_H
#define FRAME_H

#include <QVector>
#include <QColor>
#include <QString>

class Frame
{
private:
    qint32 mRows;
    qint32 mColumns;
    QVector<QVector<quint32>> mFrequencies;
    QVector<QVector<quint32>> mAmplitudes;
    QVector<QVector<QColor>> mColors;
    bool mCreationError = false;

    void setColor(qint32 row, qint32 column);
    void setError();

public:
    Frame(qint32 columns, qint32 rows);
    Frame(QString& csv, qint32 columns, qint32 rows);
    void setAmplitude(qint32 row, qint32 column, quint32 value);
    void setFrequency(qint32 row, qint32 column, quint32 value);
    QColor getColor(qint32 row, qint32 column);
    qint32 getAmplitude(qint32 row, qint32 column);
    qint32 getFrequency(qint32 row, qint32 column);
    QString toString();
    bool getError();

};

#endif // FRAME_H
