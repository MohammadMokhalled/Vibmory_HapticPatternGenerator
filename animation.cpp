#include "animation.h"
#include <QFile>
#include <QTextStream>
Animation::Animation(int rows, int columns)
{
    lock.lock();
    this->rows    = rows;
    this->columns = columns;

    this->frames.append(Frame(rows,columns));
    lock.unlock();
}

Animation::Animation(QString fileAddress)
{
    QFile file(fileAddress);
    if (file.open(QIODevice::ReadOnly))
    {
       QTextStream in(&file);
       QString setting = in.readLine();
       QStringList values = setting.split(',' , Qt::SkipEmptyParts);
       // frames.resize(values[0].toInt() + 1);
       int frameNumber = values[0].toInt();
       this->rows    = values[1].toInt();
       this->columns = values[2].toInt();

       QString frame;
       for (int i = 0; i < frameNumber; i++)
       {
           QString frame = "";
           for (int j = 0; j < rows; j++)
           {
                frame += in.readLine() + "\n";
           }

           frames.append(Frame(frame, columns, rows));
       }
       file.close();
    }
}

void Animation::addFrame()
{
    lock.lock();
    this->frames.append(Frame(this->rows,this->columns));
    lock.unlock();
}

void Animation::selectFrame(int index)
{
    lock.lock();
    currentFrame = index;
    lock.unlock();
}

int  Animation::getLen()
{
    return frames.length();
}

void Animation::setPos(int row, int column)
{
    lock.lock();
    currentRow     = row;
    currentColumn  = column;
    lock.unlock();
}

void Animation::setAmplitude(int row, int column, uint32_t value)
{
    lock.lock();
    frames[currentFrame].setAmplitude(row, column, value);
    lock.unlock();
}

void Animation::setFrequency(int row, int column, uint32_t value)
{
    lock.lock();
    frames[currentFrame].setFrequency(row, column, value);
    lock.unlock();
}

QColor Animation::getColor(int row, int column)
{
    lock.lock();
    QColor color = frames[currentFrame].getColor(row, column);
    lock.unlock();
    return color;
}

int Animation::getAmplitude(int row, int column)
{
    lock.lock();
    int amp = frames[currentFrame].getAmplitude(row, column);
    lock.unlock();
    return amp;
}

int Animation::getFrequency(int row, int column)
{
    lock.lock();
    int freq = frames[currentFrame].getFrequency(row, column);
    lock.unlock();
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
                 QString::number(rows) << ", " <<
                 QString::number(columns) << ", " <<
                 "\n";

       for (int i = 0; i < this->getLen(); i++)
       {
           stream << frames[i].toString();
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
    return frames[currentFrame].toString();
}

int Animation::getRows()
{
    return this->rows;
}

int Animation::getColumns()
{
    return this->columns;
}

void Animation::nextFrame()
{
    currentFrame++;
    if (currentFrame > frames.length() -2)
    {
        currentFrame = 0;
    }
}

int Animation::getCurrentFrameIndex()
{
    return currentFrame;
}
