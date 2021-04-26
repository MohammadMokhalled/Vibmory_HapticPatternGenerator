#include "frame.h"
#include <QRegularExpression>
Frame::Frame(int rows, int columns)
{
    this->rows = rows;
    this->columns = columns;

    frequencies.resize(rows);
    amplitudes.resize(rows);
    colors.resize(rows);

    for (int i = 0; i < rows; i++)
    {
        frequencies[i].resize(columns);
        amplitudes[i].resize(columns);
        colors[i].resize(columns);

        for (int j = 0; j < columns; j++)
        {
            frequencies[i][j] = 0;
            amplitudes[i][j] = 0;
            colors[i][j] = Qt::white;
        }
    }
}

Frame::Frame(QString csv, int columns, int rows)
{
    int r = csv.count('\n');
    int n = csv.count(',');

    if (r != rows || n != rows * columns)
    {
        throw("Error in the input parameters");
    }

    this->rows = rows;
    this->columns = columns;

    frequencies.resize(rows);
    amplitudes.resize(rows);
    colors.resize(rows);

    for (int i = 0; i < rows; i++)
    {
        frequencies[i].resize(columns);
        amplitudes[i].resize(columns);
        colors[i].resize(columns);

        for (int j = 0; j < columns; j++)
        {
            frequencies[i][j] = 0;
            amplitudes[i][j] = 0;
            colors[i][j] = Qt::white;
        }
    }

    QStringList lines = csv.split(QRegularExpression("[\n]"), Qt::SkipEmptyParts);

    for (int i = 0; i < rows; i++)
    {
        QStringList cells = lines[i].split(',' , Qt::SkipEmptyParts);
        for (int j = 0; j < columns; j++)
        {
            QStringList values = cells[j].split('-' , Qt::SkipEmptyParts);

            frequencies[i][j] = values[0].toInt();
            setAmplitude(i,j,values[1].toInt());
        }
    }
}

void Frame::setColor(int row, int column)
{
    int h = frequencies[row][column] * 200 / 22000;
    int s = (amplitudes[row][column]);
    colors[row][column].setHsv(h,s,255,255);
}

void Frame::setAmplitude(int row, int column, uint32_t value)
{
    amplitudes[row][column] = value;
    setColor(row, column);
}

void Frame::setFrequency(int row, int column, uint32_t value)
{
    frequencies[row][column] = value;
    setColor(row, column);
}

QColor Frame::getColor(int row, int column)
{
    return colors[row][column];
}

int Frame::getAmplitude(int row, int column)
{
    return amplitudes[row][column];
}

int Frame::getFrequency(int row, int column)
{
    return frequencies[row][column];
}


QString Frame::toString()
{
    QString res = "";

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < columns; j++)
        {
            res += QString::number(frequencies[i][j])
                    + "-" + QString::number(amplitudes[i][j]) + ", ";

        }
        res += "\n";
    }

    return res;
}
