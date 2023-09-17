#include "animation.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>

Animation::Animation(int rows, int columns)
{
    mLock.lock();
    qDebug() << mFrames.length();
    this->mRows    = rows;
    this->mColumns = columns;

//    this->frames.append(Frame(rows,columns));
    qDebug() << mFrames.length();
    mLock.unlock();
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
        this->mRows      = values[1].toInt();
        this->mColumns   = values[2].toInt();

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
    this->mFrames.append(Frame(this->mRows,this->mColumns));
    mLock.unlock();
}

void Animation::duplicateCurrentFrame()
{
    mLock.lock();
    this->mFrames.insert(mCurrentFrame + 1,Frame(mFrames[mCurrentFrame]));
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

       stream << QString::number(this->getLen()) << ", " <<
                 QString::number(mRows) << ", " <<
           QString::number(mColumns) << ", " <<
                 "\n";

       for (int i = 0; i < this->getLen(); i++)
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
    return this->mRows;
}

int Animation::getColumns()
{
    return this->mColumns;
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
    this->mCreationError = true;
}

bool Animation::getError()
{
    return this->mCreationError;
}

void Animation::removeCurrentFrame()
{
    mFrames.remove(mCurrentFrame);
}
