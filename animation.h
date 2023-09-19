#ifndef ANIMATION_H
#define ANIMATION_H

#include "frame.h"
#include <QVector>
#include <QMutex>
#include <QtGlobal>

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
    void setAmplitude(int row, int column, quint32 value);
    void setFrequency(int row, int column, quint32 value);
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
    quint16 mCurrentFrame = 0;
    quint16 mCurrentRow = 0;
    quint16 mCurrentColumn = 0;
    quint16 mRows;
    quint16 mColumns;
    bool mCreationError = false;

    QVector<Frame> mFrames;
    mutable QMutex mLock;

    void setError();
};

#endif // ANIMATION_H
