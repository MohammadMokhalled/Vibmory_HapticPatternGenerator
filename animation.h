#ifndef ANIMATION_H
#define ANIMATION_H

#include "frame.h"
#include <QVector>
#include <QMutex>

class QString;
class QColor;

class Animation
{
public:
    explicit Animation(int rows, int columns);
    explicit Animation(const QString& fileAddress);

    void addFrame();
    void duplicateCurrentFrame();
    void selectFrame(int index);
    int getLen() const;
    void setPos(int row, int column);
    void setAmplitude(int row, int column, uint32_t value);
    void setFrequency(int row, int column, uint32_t value);
    QColor getColor(int row, int column);
    int getAmplitude(int row, int column, int frameIndex = -1);
    int getFrequency(int row, int column, int frameIndex = -1);
    bool writeInFile(const QString& fileAddress);
    int getRows() const;
    int getColumns() const;
    QString getFrameString();
    void nextFrame();
    int getCurrentFrameIndex() const;
    bool getError() const;
    void removeCurrentFrame();

private:
    uint16_t mCurrentFrame = 0;
    uint16_t mCurrentRow = 0;
    uint16_t mCurrentColumn = 0;
    uint16_t mRows;
    uint16_t mColumns;
    bool mCreationError = false;

    QVector<Frame> mFrames;
    mutable QMutex mLock;

    void setError();
};

#endif // ANIMATION_H
