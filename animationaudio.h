#ifndef ANIMATIONAUDIO_H
#define ANIMATIONAUDIO_H

#include "animation.h"
#include <QFile>
#include <QTextStream>

#define TWOPI (double)(6.283185307179586476925286766559)

class AnimationAudio
{
public:
	AnimationAudio(Animation* animation, int frameRate, int len);
	bool generateFile(QString& fileName);

private:
	Animation*		mAnimation;
	QFile*			mAudioFile;
	QTextStream*	mStream;
	int				mFrameRate;
	int				mLength;
	const uint32_t  mSampleRate = 48000;

	bool writeHeader(QDataStream* stream);
	bool writeData(QDataStream* stream);
};

#endif // ANIMATIONAUDIO_H
