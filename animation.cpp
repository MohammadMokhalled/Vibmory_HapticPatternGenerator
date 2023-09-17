#include "animation.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>

Animation::Animation(int rows, int columns):
    mRows(rows),
    mColumns(columns)
{

}

Animation::Animation(QString fileAddress)
{
    QFile file(fileAddress);
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        QString setting = in.readLine();
        QStringList values = setting.split(',' , Qt::SkipEmptyParts);
        if (values.length() < 3)
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Error","Data is corrupted!");
            setError();
            return;
        }
        // frames.resize(values[0].toInt() + 1);
        int frameNumber = values[0].toInt();
        mRows      = values[1].toInt();
        mColumns   = values[2].toInt();

        for (int i = 0; i < frameNumber; i++)
        {
            QString frame = "";
            for (int j = 0; j < mRows; j++)
            {
                 frame += in.readLine() + "\n";
            }
            Frame newFrame = Frame(frame, mColumns, mRows);

            if (newFrame.getError())
            {
                setError();
                return;
            }

            mFrames.append(newFrame);
        }


        file.close();
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Cannot open the file");
        setError();
        return;
    }
}

void Animation::addFrame()
{
    mLock.lock();
    mFrames.append(Frame(mRows,mColumns));
    mLock.unlock();
}

void Animation::duplicateCurrentFrame()
{
    mLock.lock();
    mFrames.insert(mCurrentFrame + 1,Frame(mFrames[mCurrentFrame]));
    mLock.unlock();
}

void Animation::selectFrame(int index)
{
    mLock.lock();
    mCurrentFrame = index;
    mLock.unlock();
}

int  Animation::getLen()
{
    return mFrames.length();
}

void Animation::setPos(int row, int column)
{
    mLock.lock();
    mCurrentRow     = row;
    mCurrentColumn  = column;
    mLock.unlock();
}

void Animation::setAmplitude(int row, int column, uint32_t value)
{
    mLock.lock();
    mFrames[mCurrentFrame].setAmplitude(row, column, value);
    mLock.unlock();
}

void Animation::setFrequency(int row, int column, uint32_t value)
{
    mLock.lock();
    mFrames[mCurrentFrame].setFrequency(row, column, value);
    mLock.unlock();
}

QColor Animation::getColor(int row, int column)
{
    mLock.lock();
    QColor color = mFrames[mCurrentFrame].getColor(row, column);
    mLock.unlock();
    return color;
}

int Animation::getAmplitude(int row, int column, int frameIndex)
{
    if (frameIndex == -1)
    {
        frameIndex = mCurrentFrame;
    }
    mLock.lock();
    int amp = mFrames[frameIndex].getAmplitude(row, column);
    mLock.unlock();
    return amp;
}

int Animation::getFrequency(int row, int column, int frameIndex)
{
    if (frameIndex == -1)
    {
        frameIndex = mCurrentFrame;
    }
    mLock.lock();
    int freq = mFrames[frameIndex].getFrequency(row, column);
    mLock.unlock();
    return freq;
}

QString Animation::writeInFile(QString address)
{
    QString message = "";
    QFile file(address);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
       QTextStream stream(&file);

       stream << QString::number(getLen()) << ", " <<
                 QString::number(mRows) << ", " <<
           QString::number(mColumns) << ", " <<
                 "\n";

       for (int i = 0; i < getLen(); i++)
       {
            stream << mFrames[i].toString();
       }
       file.close();

       message = "File is exported sucessfully";

    }
    else
    {
        message = "Cannot open or create the file.";
    }

    return message;
}

QString Animation::getFrameString()
{
    return mFrames[mCurrentFrame].toString();
}

int Animation::getRows()
{
    return mRows;
}

int Animation::getColumns()
{
    return mColumns;
}

void Animation::nextFrame()
{
    mCurrentFrame++;
    if (mCurrentFrame > mFrames.length() -1)
    {
        mCurrentFrame = 0;
    }
}

int Animation::getCurrentFrameIndex()
{
    return mCurrentFrame;
}

void Animation::setError()
{
    mCreationError = true;
}

bool Animation::getError()
{
    return mCreationError;
}

void Animation::removeCurrentFrame()
{
    mFrames.remove(mCurrentFrame);
}
