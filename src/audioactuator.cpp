#include "audioactuator.h"
#include <QByteArray>
#include <QDataStream>
#include <cmath>
#include <QDir>
#include <QDebug>

#define TWOPI (double)(6.283185307179586476925286766559)
#define DEFAULT_FILE_NAME (QDir::tempPath() + QString("/animation.wav"))

AudioActuator::AudioActuator(Animation *animation, qint32 frameRate, qint32 duration, quint32 sampleRate):
    mAnimation(animation),
    mFrameRate(frameRate),
    mDuration(duration),
    mSampleRate(sampleRate),
    mFileName(DEFAULT_FILE_NAME),
    mAudioOutput(new QAudioOutput()), 
    mAudioFile(new QFile(mFileName))
{
}

bool AudioActuator::generateFile(const QString &fileName)
{
    QString tempFileName = fileName;
    if (tempFileName.isEmpty())
    {
        tempFileName = mFileName;
    }

    mAudioFile->setFileName(mFileName);
    if(mAudioFile->open(QIODevice::WriteOnly | QFile::Truncate))
    {
        QDataStream stream(mAudioFile);
        stream.setByteOrder(QDataStream::LittleEndian);

        writeData(&stream);
    }
    mAudioFile->close();

    return true;
}

void AudioActuator::setFrameRate(qint32 frameRate)
{
    mFrameRate = frameRate;
}

void AudioActuator::setDuration(qint32 duration)
{
    mDuration = duration;
}

void AudioActuator::setSampleRate(quint32 sampleRate)
{
    mSampleRate = sampleRate;
}

bool AudioActuator::prepare()
{
    return generateFile();
}

bool AudioActuator::play()
{   
    mAudioFile->open(QIODevice::ReadOnly);

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(mSampleRate);
    format.setChannelCount(mAnimation->getSize().height() *
                           mAnimation->getSize().width());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if(!info.isFormatSupported(format))
    {
        qWarning()
            << "Raw audio format not supported by backend, cannot play audio.";
    }
    delete mAudioOutput;
    mAudioOutput = new QAudioOutput(format);
    mAudioOutput->start(mAudioFile);
    return true;
}

bool AudioActuator::stop()
{
    mAudioOutput->stop();
    mAudioFile->close();
    return true;
}

bool AudioActuator::writeHeader(QDataStream *stream)
{

    *stream << (quint8)'R' << (quint8)'I' << (quint8)'F' << (quint8)'F';
    
    quint32 fileSize = mAnimation->getSize().width() * mAnimation->getSize().height() * mDuration * mSampleRate * 2 + 44;
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

bool AudioActuator::writeData(QDataStream *stream)
{
    quint32 maxValue = mSampleRate * mDuration;
    quint32 frameLen = mSampleRate / mFrameRate;
    quint32 numberOfFrames = mAnimation->getLen();

    for (quint32 x = 0; x < maxValue; x++)
    {
        double t = static_cast<double>(x) / mSampleRate;
        quint16 frameIndex = (x / frameLen) % (numberOfFrames);
        
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
