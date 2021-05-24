#ifndef ANIMATION_H
#define ANIMATION_H

#include "frame.h"
#include "QMutex"
#include "QString"

class Animation
{
private:
    uint16_t currentFrame   = 0;
    uint16_t currentRow     = 0;
    uint16_t currentColumn  = 0;
    uint16_t rows;
    uint16_t columns;
    bool creationError = false;

    QVector<Frame> frames;
    QMutex lock;
    void setError();

public:
    Animation(int rows, int columns);
    Animation(QString fileAddress);
    void addFrame();

    void duplicateCurrentFrame();
    void selectFrame(int index);
    int  getLen();
    void setPos(int row, int column);
    void setAmplitude(int row, int column, uint32_t value);
    void setFrequency(int row, int column, uint32_t value);
    QColor getColor(int row, int column);
    int getAmplitude(int row, int column, int frameIndex = -1);
    int getFrequency(int row, int column, int frameIndex = -1);
    QString writeInFile(QString address);
    int getRows();
    int getColumns();
    QString getFrameString();
    void nextFrame();
    int getCurrentFrameIndex();
    bool getError();
    void removeCurrentFrame();
};



#endif // ANIMATION_H
