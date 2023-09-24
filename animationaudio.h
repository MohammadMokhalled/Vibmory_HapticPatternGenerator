#ifndef ANIMATIONAUDIO_H
#define ANIMATIONAUDIO_H

#include "animation.h"
#include <QFile>
#include <QTextStream>
#include <QtGlobal>

#define TWOPI (double)(6.283185307179586476925286766559)
/**
 * @class AnimationAudio
 * Class for handling audio in animations.
 */
class AnimationAudio
{
public:
	/**
	 * Constructor for the AnimationAudio class.
	 * @param animation Pointer to the Animation object.
	 * @param frameRate The frame rate of the animation.
	 * @param len Number of frames in the animation.
	 * @param sampleRate The sample rate of the audio file(it is optional, default is 48000).
	 */
    AnimationAudio(Animation* animation, qint32 frameRate, qint32 len, quint32 sampleRate = 48000);

	/**
	 * Generates audio file using the given file name.
	 * @param fileName The name of the file to generate.
	 * @return True if the file generation is successful, false otherwise.
	 */
	bool generateFile(QString& fileName);


private:
	Animation*		mAnimation;
	QFile*			mAudioFile;
	QTextStream*	mStream;
    qint16			mFrameRate;
    qint16			mLength;
    quint32   		mSampleRate = 48000;

	/**
	 * Writes the header of audio file.
	 *
	 * @param stream - The data stream to write the header to the given data stream of audio file.
	 * @return True if the header was successfully written, false otherwise.
	 */
	bool writeHeader(QDataStream* stream);

	/**
	 * Writes the audio data to the given data stream of audio file.
	 *
	 * @param stream - The data stream to write the data to.
	 * @return True if the data was successfully written, false otherwise.
	 */
	bool writeData(QDataStream* stream);
};

#endif // ANIMATIONAUDIO_H
