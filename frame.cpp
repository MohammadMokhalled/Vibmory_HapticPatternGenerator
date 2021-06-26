#include "frame.h"
#include <QRegularExpression>
#include <QMessageBox>

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

    if (r != rows || n != rows * columns * 2)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Frame data is not correct!");
        setError();
        throw;
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
    if (lines.length() != rows)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Frame data is not correct!");
        setError();
        return;
    }

    for (int i = 0; i < rows; i++)
    {
        QStringList cells = lines[i].split(", " , Qt::SkipEmptyParts);
        if (cells.length() != (columns*2))
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Error","Frame data is not correct!");
            setError();
            return;

        }
        for (int j = 0; j < (columns * 2); j+=2)
        {
            //QStringList values = cells[j].split('-' , Qt::SkipEmptyParts);

            frequencies[i][j/2] = cells[j].toInt();
            setAmplitude(i,j/2,cells[j+1].toInt());
        }
    }
}

void Frame::setColor(int row, int column)
{
    int h = frequencies[row][column] * 200 / 22000;
    int s = (amplitudes[row][column]) * 255 / 32767;
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
                    + ", " + QString::number(amplitudes[i][j]) + ", ";

        }
        res += "\n";
    }

    return res;
}

void Frame::setError()
{
    this->creationError = true;
}

bool Frame::getError()
{
    return this->creationError;
}
