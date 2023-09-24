#include "frame.h"
#include <QRegularExpression>
#include <QMessageBox>

Frame::Frame(const QSize& size):
    mSize(size), 
    mCreationError(false)
{
    mFrequencies.resize(mSize.height());
    mAmplitudes.resize(mSize.height());
    mColors.resize(mSize.height());

    for (qint32 i = 0; i < mSize.height(); i++)
    {
        mFrequencies[i].resize(mSize.width());
        mAmplitudes[i].resize(mSize.width());
        mColors[i].resize(mSize.width());

        for (qint32 j = 0; j < mSize.width(); j++)
        {
            mFrequencies[i][j] = 0;
            mAmplitudes[i][j] = 0;
            mColors[i][j] = Qt::white;
        }
    }
}

Frame::Frame(const QString& csv, const QSize& size)
    : mSize(size), mCreationError(false)
{
    qint32 r = csv.count('\n');
    qint32 n = csv.count(',');

    if (r != mSize.height() || n != mSize.height() * mSize.width() * 2)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Frame data is not correct!");
        setError();
        throw;
    }

    mFrequencies.resize(mSize.height());
    mAmplitudes.resize(mSize.height());
    mColors.resize(mSize.height());

    for (qint32 i = 0; i < mSize.height(); i++)
    {
        mFrequencies[i].resize(mSize.width());
        mAmplitudes[i].resize(mSize.width());
        mColors[i].resize(mSize.width());

        for (qint32 j = 0; j < mSize.width(); j++)
        {
            mFrequencies[i][j] = 0;
            mAmplitudes[i][j] = 0;
            mColors[i][j] = Qt::white;
        }
    }

    QStringList lines = csv.split(QRegularExpression("[\n]"), Qt::SkipEmptyParts);
    if (lines.length() != mSize.height())
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Frame data is not correct!");
        setError();
        return;
    }

    for (qint32 i = 0; i < mSize.height(); i++)
    {
        QStringList cells = lines[i].split(", " , Qt::SkipEmptyParts);
        if (cells.length() != (mSize.width() * 2))
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Error","Frame data is not correct!");
            setError();
            return;
        }
        for (qint32 j = 0; j < (mSize.width() * 2); j+=2)
        {
            mFrequencies[i][j/2] = cells[j].toInt();
            setAmplitude(QPoint(j/2, i), cells[j+1].toInt());
        }
    }
}

void Frame::setColor(const QPoint& pos)
{
    if (!isValidIndex(pos)) return;
    qint32 h = mFrequencies[pos.y()][pos.x()] * 200 / 22000;
    qint32 s = mAmplitudes[pos.y()][pos.x()] * 255 / 32767;
    mColors[pos.y()][pos.x()].setHsv(h, s, 255, 255);
}

void Frame::setAmplitude(const QPoint& pos, quint32 value)
{
    if (!isValidIndex(pos)) return;
    mAmplitudes[pos.y()][pos.x()] = value;
    setColor(pos);
}

void Frame::setFrequency(const QPoint& pos, quint32 value)
{
    if (!isValidIndex(pos)) return;
    mFrequencies[pos.y()][pos.x()] = value;
    setColor(pos);
}

QColor Frame::getColor(const QPoint& pos) const
{
    return isValidIndex(pos) ? mColors[pos.y()][pos.x()] : QColor();
}

qint32 Frame::getAmplitude(const QPoint& pos) const
{
    return isValidIndex(pos) ? mAmplitudes[pos.y()][pos.x()] : -1;
}

qint32 Frame::getFrequency(const QPoint& pos) const
{
    return isValidIndex(pos) ? mFrequencies[pos.y()][pos.x()] : -1;
}

QString Frame::toString() const
{
    QString res = "";
    for (qint32 i = 0; i < mSize.height(); i++)
    {
        for (qint32 j = 0; j < mSize.width(); j++)
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

bool Frame::getError() const
{
    return mCreationError;
}

bool Frame::isValidIndex(const QPoint& pos) const
{
    return pos.x() >= 0 && pos.x() < mSize.width() && pos.y() >= 0 && pos.y() < mSize.height();
}
