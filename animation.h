#ifndef ANIMATION_H
#define ANIMATION_H

#include "frame.h"
#include "QMutex"
#include "QString"

class Animation
{
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

private:
    uint16_t mCurrentFrame   = 0;
    uint16_t mCurrentRow     = 0;
    uint16_t mCurrentColumn  = 0;
    uint16_t mRows;
    uint16_t mColumns;
    bool mCreationError = false;

    QVector<Frame> mFrames;
    QMutex mLock;

    void setError();

};



#endif // ANIMATION_H
