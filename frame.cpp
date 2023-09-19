#include "frame.h"
#include <QRegularExpression>
#include <QMessageBox>

Frame::Frame(qint32 rows, qint32 columns):
    mRows(rows),
    mColumns(columns),
    mCreationError(false)
{
    mFrequencies.resize(rows);
    mAmplitudes.resize(rows);
    mColors.resize(rows);

    for (qint32 i = 0; i < rows; i++)
    {
        mFrequencies[i].resize(columns);
        mAmplitudes[i].resize(columns);
        mColors[i].resize(columns);

        for (qint32 j = 0; j < columns; j++)
        {
            mFrequencies[i][j] = 0;
            mAmplitudes[i][j] = 0;
            mColors[i][j] = Qt::white;
        }
    }
}

Frame::Frame(QString& csv, qint32 columns, qint32 rows):
    mRows(rows),
    mColumns(columns)
{
    qint32 r = csv.count('\n');
    qint32 n = csv.count(',');

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

    for (qint32 i = 0; i < mRows; i++)
    {
        mFrequencies[i].resize(mColumns);
        mAmplitudes[i].resize(mColumns);
        mColors[i].resize(mColumns);

        for (qint32 j = 0; j < mColumns; j++)
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

    for (qint32 i = 0; i < mRows; i++)
    {
        QStringList cells = lines[i].split(", " , Qt::SkipEmptyParts);
        if (cells.length() != (mColumns *2))
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Error","Frame data is not correct!");
            setError();
            return;

        }
        for (qint32 j = 0; j < (mColumns * 2); j+=2)
        {
            //QStringList values = cells[j].split('-' , Qt::SkipEmptyParts);

            mFrequencies[i][j/2] = cells[j].toInt();
            setAmplitude(i,j/2,cells[j+1].toInt());
        }
    }
}

void Frame::setColor(qint32 row, qint32 column)
{
    qint32 h = mFrequencies[row][column] * 200 / 22000;
    qint32 s = (mAmplitudes[row][column]) * 255 / 32767;
    mColors[row][column].setHsv(h,s,255,255);
}

void Frame::setAmplitude(qint32 row, qint32 column, quint32 value)
{
    mAmplitudes[row][column] = value;
    setColor(row, column);
}

void Frame::setFrequency(qint32 row, qint32 column, quint32 value)
{
    mFrequencies[row][column] = value;
    setColor(row, column);
}

QColor Frame::getColor(qint32 row, qint32 column)
{
    return mColors[row][column];
}

qint32 Frame::getAmplitude(qint32 row, qint32 column)
{
    return mAmplitudes[row][column];
}

qint32 Frame::getFrequency(qint32 row, qint32 column)
{
    return mFrequencies[row][column];
}

QString Frame::toString()
{
    QString res = "";

    for (qint32 i = 0; i < mRows; i++)
    {
        for (qint32 j = 0; j < mColumns; j++)
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
