#include "animationaudio.h"
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <cmath>

AnimationAudio::AnimationAudio(Animation *animation, int frameRate, int len):
    mAnimation(animation),
    mFrameRate(frameRate),
    mLength(len)
{

}

bool AnimationAudio::writeHeader(QDataStream *stream)
{

    *stream << (uint8_t)'R' << (uint8_t)'I' << (uint8_t)'F' << (uint8_t)'F';
    
    uint32_t fileSize = mAnimation->getRows() * mAnimation->getColumns() * mLength * mSampleRate * 2 + 44;
    *stream << fileSize;

    *stream << (uint8_t)'W' << (uint8_t)'A' << (uint8_t)'V' << (uint8_t)'E';

    *stream << (uint8_t)'f' << (uint8_t)'m' << (uint8_t)'t' << (uint8_t)' ';

    uint32_t formatdata = 16;
    *stream << formatdata;

    uint16_t format = 1;
    *stream << format;
    
    uint16_t numberOfChannels =  mAnimation->getRows() * mAnimation->getColumns();
    *stream << numberOfChannels;

    *stream << mSampleRate;

    uint32_t dataRate = (mSampleRate * numberOfChannels * 16) / 8;
    *stream << dataRate;

    uint16_t blockAllign = (numberOfChannels * 16) / 8;
    *stream << blockAllign;


    uint16_t bitsPerSample = 16;
    *stream << bitsPerSample;

    *stream << 'd' << 'a' << 't' << 'a';

    uint32_t subchunk2Size = fileSize - 44;
    *stream << subchunk2Size;

    return true;
}

bool AnimationAudio::writeData(QDataStream *stream)
{
    uint32_t maxValue = mSampleRate * mLength;
    uint32_t frameLen = mSampleRate / mFrameRate;
    uint32_t numberOfFrames = mAnimation->getLen();
    uint16_t numberOfActuators = mAnimation->getRows() * mAnimation->getColumns();


    for (uint32_t x = 0; x < maxValue; x++)
    {
        double t = static_cast<double>(x) / mSampleRate;
        uint16_t frameIndex = (x / frameLen) % (numberOfFrames -1);
        
        for (int i = 0; i < mAnimation->getRows(); i++)
        {
            for (int j = 0; j < mAnimation->getColumns(); j++)
            {
                double w = (mAnimation->getFrequency(i,j,frameIndex)  * TWOPI);
                int16_t amplitude = mAnimation->getAmplitude(i,j,frameIndex);

                int16_t value = amplitude * sin(w * t);
                *stream << value;
            }
        }
    }
    return true;
}

bool AnimationAudio::generateFile(QString& fileName)
{
    QString message = "";
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly|QFile::Truncate))
    {
       QDataStream stream(&file);
       stream.setByteOrder(QDataStream::LittleEndian);

       writeData(&stream);

    }
    file.close();

    return true;
}
