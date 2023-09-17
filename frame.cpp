#include "frame.h"
#include <QRegularExpression>
#include <QMessageBox>

Frame::Frame(int rows, int columns):
    mRows(rows),
    mColumns(columns)
{
    mFrequencies.resize(rows);
    mAmplitudes.resize(rows);
    mColors.resize(rows);

    for (int i = 0; i < rows; i++)
    {
        mFrequencies[i].resize(columns);
        mAmplitudes[i].resize(columns);
        mColors[i].resize(columns);

        for (int j = 0; j < columns; j++)
        {
            mFrequencies[i][j] = 0;
            mAmplitudes[i][j] = 0;
            mColors[i][j] = Qt::white;
        }
    }
}

Frame::Frame(QString& csv, int columns, int rows):
    mRows(rows),
    mColumns(columns)
{
    int r = csv.count('\n');
    int n = csv.count(',');

    if (r != mRows || n != mRows * mColumns * 2)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Frame data is not correct!");
        setError();
        throw;
    }

    mFrequencies.resize(mRows);
    mAmplitudes.resize(mRows);
    mColors.resize(mRows);

    for (int i = 0; i < mRows; i++)
    {
        mFrequencies[i].resize(mColumns);
        mAmplitudes[i].resize(mColumns);
        mColors[i].resize(mColumns);

        for (int j = 0; j < mColumns; j++)
        {
            mFrequencies[i][j] = 0;
            mAmplitudes[i][j] = 0;
            mColors[i][j] = Qt::white;
        }
    }

    QStringList lines = csv.split(QRegularExpression("[\n]"), Qt::SkipEmptyParts);
    if (lines.length() != mRows)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Frame data is not correct!");
        setError();
        return;
    }

    for (int i = 0; i < mRows; i++)
    {
        QStringList cells = lines[i].split(", " , Qt::SkipEmptyParts);
        if (cells.length() != (mColumns *2))
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Error","Frame data is not correct!");
            setError();
            return;

        }
        for (int j = 0; j < (mColumns * 2); j+=2)
        {
            //QStringList values = cells[j].split('-' , Qt::SkipEmptyParts);

            mFrequencies[i][j/2] = cells[j].toInt();
            setAmplitude(i,j/2,cells[j+1].toInt());
        }
    }
}

void Frame::setColor(int row, int column)
{
    int h = mFrequencies[row][column] * 200 / 22000;
    int s = (mAmplitudes[row][column]) * 255 / 32767;
    mColors[row][column].setHsv(h,s,255,255);
}

void Frame::setAmplitude(int row, int column, uint32_t value)
{
    mAmplitudes[row][column] = value;
    setColor(row, column);
}

void Frame::setFrequency(int row, int column, uint32_t value)
{
    mFrequencies[row][column] = value;
    setColor(row, column);
}

QColor Frame::getColor(int row, int column)
{
    return mColors[row][column];
}

int Frame::getAmplitude(int row, int column)
{
    return mAmplitudes[row][column];
}

int Frame::getFrequency(int row, int column)
{
    return mFrequencies[row][column];
}


QString Frame::toString()
{
    QString res = "";

    for (int i = 0; i < mRows; i++)
    {
        for (int j = 0; j < mColumns; j++)
        {
            res += QString::number(mFrequencies[i][j])
                    + ", " + QString::number(mAmplitudes[i][j]) + ", ";

        }
        res += "\n";
    }

    return res;
}

void Frame::setError()
{
    mCreationError = true;
}

bool Frame::getError()
{
    return mCreationError;
}
