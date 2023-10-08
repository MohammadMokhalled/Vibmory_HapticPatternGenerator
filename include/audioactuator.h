#ifndef AUDIOACTUATOR_H
#define AUDIOACTUATOR_H

#include <QFile>
#include <QTextStream>
#include <QtGlobal>
#include "animation.h"
#include "iactuatorcontroller.h"
#include <QAudioOutput>

/**
 * @class AudioActuator
 * Class for handling audio in animations.
 */
class AudioActuator : public IActuatorController
{
public:
    /**
     * Constructor for the AudioActuator class.
     * @param animation Pointer to the Animation object.
     * @param frameRate The frame rate of the animation.
     * @param duration The duration of audio in seconds.
     * @param sampleRate The sample rate of the audio file(it is optional,
     * default is 48000).
     */
    AudioActuator(Animation *animation, qint32 frameRate = 1,
                  qint32 duration = 0, quint32 sampleRate = 48000);

    /**
     * Set the frame rate of the animation.
     * @param frameRate The frame rate of the animation.
     */
    void setFrameRate(qint32 frameRate) override;

    /**
     * Set the duration of audio in seconds.
     * @param duration The duration of audio in seconds.
     */
    void setDuration(qint32 duration) override;

    /**
     * Set the sample rate of the audio file.
     * @param sampleRate The sample rate of the audio file.
     */
    void setSampleRate(quint32 sampleRate);

    /**
     * Prepare the object for controlling the actuators.
     * @return True if the object is successfully prepared, false otherwise.
     */
    bool prepare() override;

    /**
     * Starts controlling the actuators.
     * @return True if the file is successfully played, false otherwise.
     */
    bool play() override;

    /**
     * Stops controlling the actuators.
     * @return True if the file is successfully stopped, false otherwise.
     */
    bool stop() override;

private:
    Animation *mAnimation;
    qint32 mFrameRate;
    qint32 mDuration;
    quint32 mSampleRate;
    const QString mFileName;
    QAudioOutput *mAudioOutput;
    QFile *mAudioFile;

    /**
     * Writes the header of audio file.
     *
     * @param stream - The data stream to write the header to the given data
     * stream of audio file.
     * @return True if the header was successfully written, false otherwise.
     */
    bool writeHeader(QDataStream *stream);

    /**
     * Writes the audio data to the given data stream of audio file.
     *
     * @param stream - The data stream to write the data to.
     * @return True if the data was successfully written, false otherwise.
     */
    bool writeData(QDataStream *stream);

    /**
     * Generates audio file using the given file name.
     * @param fileName The name of the file to generate.
     * @return True if the file generation is successful, false otherwise.
     */
    bool generateFile(const QString &fileName = QString());
};

#endif // ANIMATIONAUDIO_H
