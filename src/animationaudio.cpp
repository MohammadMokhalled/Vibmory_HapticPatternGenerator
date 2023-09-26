#include "animationaudio.h"
#include <QByteArray>
#include <QDataStream>
#include <cmath>

AnimationAudio::AnimationAudio(Animation *animation, qint32 frameRate, qint32 len, quint32 sampleRate):
    mAnimation(animation),
    mFrameRate(frameRate),
    mLength(len),
    mSampleRate(sampleRate)
{

}

bool AnimationAudio::writeHeader(QDataStream *stream)
{

    *stream << (quint8)'R' << (quint8)'I' << (quint8)'F' << (quint8)'F';
    
    quint32 fileSize = mAnimation->getSize().width() * mAnimation->getSize().height() * mLength * mSampleRate * 2 + 44;
    *stream << fileSize;

    *stream << (quint8)'W' << (quint8)'A' << (quint8)'V' << (quint8)'E';

    *stream << (quint8)'f' << (quint8)'m' << (quint8)'t' << (quint8)' ';

    quint32 formatdata = 16;
    *stream << formatdata;

    quint16 format = 1;
    *stream << format;
    
    quint16 numberOfChannels =  mAnimation->getSize().width() * mAnimation->getSize().height();
    *stream << numberOfChannels;

    *stream << mSampleRate;

    quint32 dataRate = (mSampleRate * numberOfChannels * 16) / 8;
    *stream << dataRate;

    quint16 blockAllign = (numberOfChannels * 16) / 8;
    *stream << blockAllign;


    quint16 bitsPerSample = 16;
    *stream << bitsPerSample;

    *stream << 'd' << 'a' << 't' << 'a';

    quint32 subchunk2Size = fileSize - 44;
    *stream << subchunk2Size;

    return true;
}

bool AnimationAudio::writeData(QDataStream *stream)
{
    quint32 maxValue = mSampleRate * mLength;
    quint32 frameLen = mSampleRate / mFrameRate;
    quint32 numberOfFrames = mAnimation->getLen();

    for (quint32 x = 0; x < maxValue; x++)
    {
        double t = static_cast<double>(x) / mSampleRate;
        quint16 frameIndex = (x / frameLen) % (numberOfFrames -1);
        
        for (qint32 i = 0; i < mAnimation->getSize().width(); i++)
        {
            for (qint32 j = 0; j < mAnimation->getSize().height(); j++)
            {
                double w = (mAnimation->getFrequency(QPoint(i,j),frameIndex)  * TWOPI);
                int16_t amplitude = mAnimation->getAmplitude(QPoint(i,j),frameIndex);

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
