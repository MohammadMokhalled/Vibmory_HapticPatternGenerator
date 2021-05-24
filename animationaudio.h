#ifndef ANIMATIONAUDIO_H
#define ANIMATIONAUDIO_H

#include "animation.h"
#include <QFile>
#include <QTextStream>

#define TWOPI (double)(6.283185307179586476925286766559)

class AnimationAudio
{
private:
    Animation       *animation;
    QFile           *audioFile;
    QTextStream     *stream;
    int             frameRate;
    int             length;
    const uint32_t  sampleRate = 48000;


    bool writeHeader(QDataStream *stream);
    bool writeData(QDataStream *stream);

public:
    AnimationAudio(Animation *anim, int frameRate, int len);
    bool generateFile(QString fileName);

};

#endif // ANIMATIONAUDIO_H
