#include "animation.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

Animation::Animation(const QSize& size)
    : mSize(size),
      mCurrentFrame(0),
      mCurrentPos(0, 0),
      mCreationError(false)
{

}

Animation::Animation(const QString& fileAddress)
    : mCreationError(false)
{
    QFile file(fileAddress);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(nullptr, "Error", "Cannot open the file");
        setError();
        return;
    }
    QTextStream in(&file);
    QString setting = in.readLine();
    QStringList values = setting.split(',' , Qt::SkipEmptyParts);
    if (values.length() < 3)
    {
        QMessageBox::critical(nullptr, "Error", "Data is corrupted!");
        setError();
        return;
    }
    int frameNumber = values[0].toInt();
    mSize = QSize(values[1].toInt(), values[2].toInt());
    for (int i = 0; i < frameNumber; ++i)
    {
        QString frame;
        for (int j = 0; j < mSize.height(); ++j)
        {
            frame += in.readLine() + "\n";
        }
        Frame newFrame(frame, mSize);
        if (newFrame.getError())
        {
            setError();
            return;
        }
        mFrames.append(newFrame);
    }
    file.close();
}

void Animation::addFrame()
{
    QMutexLocker locker(&mLock);
    mFrames.append(Frame(mSize));
}

void Animation::duplicateCurrentFrame()
{
    QMutexLocker locker(&mLock);
    mFrames.insert(mCurrentFrame + 1, Frame(mFrames.at(mCurrentFrame)));
}

void Animation::selectFrame(int index)
{
    QMutexLocker locker(&mLock);
    mCurrentFrame = index;
}

int Animation::getLen() const
{
    QMutexLocker locker(&mLock);
    return mFrames.length();
}

void Animation::setPos(const QPoint& pos)
{
    QMutexLocker locker(&mLock);
    mCurrentPos = pos;
}

void Animation::setAmplitude(const QPoint& pos, quint32 value)
{
    QMutexLocker locker(&mLock);
    mFrames[mCurrentFrame].setAmplitude(pos, value);
}

void Animation::setFrequency(const QPoint& pos, quint32 value)
{
    QMutexLocker locker(&mLock);
    mFrames[mCurrentFrame].setFrequency(pos, value);
}

QColor Animation::getColor(const QPoint& pos)
{
    QMutexLocker locker(&mLock);
    QColor color = mFrames[mCurrentFrame].getColor(pos);
    return color;
}

int Animation::getAmplitude(const QPoint& pos, int frameIndex)
{
    if (frameIndex == -1)
    {
        frameIndex = mCurrentFrame;
    }
    return mFrames[frameIndex].getAmplitude(pos);
}

int Animation::getFrequency(const QPoint& pos, int frameIndex)
{
    if (frameIndex == -1)
    {
        frameIndex = mCurrentFrame;
    }
    return mFrames[frameIndex].getFrequency(pos);
}

bool Animation::writeInFile(const QString& fileAddress)
{
    QFile file(fileAddress);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    QTextStream stream(&file);
    stream << QString::number(getLen()) << ", " << QString::number(mSize.height()) << ", " << QString::number(mSize.width()) << "\n";
    for (const auto& frame : qAsConst(mFrames))
    {
        stream << frame.toString();
    }
    file.close();
    return true;
}

QString Animation::getFrameString()
{
    return mFrames[mCurrentFrame].toString();
}

QSize Animation::getSize() const
{
    return mSize;
}

void Animation::nextFrame()
{
    mCurrentFrame++;
    if (mCurrentFrame >= mFrames.size())
    {
        mCurrentFrame = 0;
    }
}

int Animation::getCurrentFrameIndex() const
{
    return mCurrentFrame;
}

void Animation::setError()
{
    mCreationError = true;
}

bool Animation::getError() const
{
    return mCreationError;
}

void Animation::removeCurrentFrame()
{
    QMutexLocker locker(&mLock);
    if (mFrames.isEmpty()) return;
    mFrames.removeAt(mCurrentFrame);
    if (mCurrentFrame >= mFrames.size())
    {
        mCurrentFrame = mFrames.size() - 1;
    }
}
