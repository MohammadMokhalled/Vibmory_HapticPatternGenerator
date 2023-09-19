#ifndef ANIMATIONAUDIO_H
#define ANIMATIONAUDIO_H

#include "animation.h"
#include <QFile>
#include <QTextStream>
#include <QtGlobal>

#define TWOPI (double)(6.283185307179586476925286766559)

class AnimationAudio
{
public:
	AnimationAudio(Animation* animation, qint32 frameRate, qint32 len);
	bool generateFile(QString& fileName);

private:
	Animation*		mAnimation;
	QFile*			mAudioFile;
	QTextStream*	mStream;
    qint16			mFrameRate;
    qint16			mLength;
    const quint32   mSampleRate = 48000;

	bool writeHeader(QDataStream* stream);
	bool writeData(QDataStream* stream);
};

#endif // ANIMATIONAUDIO_H
