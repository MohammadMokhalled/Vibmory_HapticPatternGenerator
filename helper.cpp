#include "helper.h"

#include <QPainter>
#include <QPaintEvent>
#include <QWidget>
#include <QDebug>

Helper::Helper(Animation* animation, int rows, int columns):
    mAnimation(animation),
    mRows(rows),
    mColumns(columns),
    mBackgroundBrush(QColor(255, 255, 255)),
    mForegroundBrush(QColor(0, 0, 0)),
    mBoldPen(Qt::black),
    mTextPen(Qt::white),
    mTextFont("Arial", 12, QFont::Bold),
    mIsPlaying(false),
    mTabIndexBeforePlay(0),
    mSelectedRow(-1),
    mSelectedColumn(-1),
    mFirstFramePlay(false),
    mSelectedPosition(false),
    mCellWidth(0),
    mCellHeight(0),
    mPaintingState(0),
    mAudio(nullptr)
{
    QLinearGradient gradient(QPointF(50, -20), QPointF(80, 20));
    gradient.setColorAt(0.0, Qt::white);
    gradient.setColorAt(1.0, QColor(0xa6, 0xce, 0x39));
}

void Helper::drawBackground(QPainter *painter)
{
    int width = painter->device()->width();
    int height = painter->device()->height();
    const QRect rect = QRect(QPoint(0,0), QPoint(width,height));
    painter->fillRect(rect, mBackgroundBrush);

    mCellWidth = width / mColumns;
    mCellHeight = height / mRows;

    for (int i = 1; i < mColumns; i++)
    {
        painter->drawLine(i*mCellWidth,0,i*mCellWidth,height);
    }

    for (int i = 1; i < mRows; i++)
    {
        painter->drawLine(0,i*mCellHeight,width,i*mCellHeight);
    }
}

void Helper::drawColors(QPainter *painter)
{
    for (int i = 0; i < mRows; i++)
    {
        for (int j = 0; j < mColumns; j++)
        {
            if (mAnimation->getColor(QPoint(i,j)) != Qt::white)
            {
                mForegroundBrush = QBrush(mAnimation->getColor(QPoint(i,j)));
                const QRect rect = QRect(j*mCellWidth+2, i*mCellHeight+2, mCellWidth-4, mCellHeight-4);
                painter->fillRect(rect, mForegroundBrush);
            }
        }
    }
}

void Helper::playAudio()
{
    mSourceFile.setFileName("file.wav"); //C:/Users/mmokh/OneDrive/Documents/build-Sensors_CPP-Desktop_Qt_6_0_3_MSVC2019_64bit-Debug/
    mSourceFile.open(QIODevice::ReadOnly);

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(48000);
    format.setChannelCount(mAnimation->getSize().height() * mAnimation->getSize().width());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
    }

    mAudio = new QAudioOutput(format);
    mAudio->start(&mSourceFile);
}

void Helper::startPlay()
{
    mTabIndexBeforePlay = mAnimation->getCurrentFrameIndex();
    mAnimation->selectFrame(0);
    mIsPlaying = true;
    mFirstFramePlay = true;
}

void Helper::stopPlay()
{
    mIsPlaying = false;
    mAnimation->selectFrame(mTabIndexBeforePlay);
    mAudio->stop();
    mSourceFile.close();
}


void Helper::paint(QPainter *painter, QPaintEvent *event)
{
    mLock.lock();
    if (mFirstFramePlay)
    {
        playAudio();
        mFirstFramePlay = false;
    }
    drawBackground(painter);

    drawColors(painter);

    if (mSelectedPosition) //clicked
    {
        painter->setPen(mBoldPen);
        painter->drawRect(mSelectedColumn * mCellWidth, mSelectedRow * mCellHeight, mCellWidth, mCellHeight);
    }

    if (mIsPlaying)
    {
        mAnimation->nextFrame();
    }

    mLock.unlock();
}

bool Helper::setPaintingState(int state)
{
  mLock.lock();
  mPaintingState = state;
  mLock.unlock();
  return true;
}

void Helper::selectCell(int x, int y)
{
  mLock.lock();
  mSelectedPosition = true;
  mSelectedColumn = x/mCellWidth;
  mSelectedRow = y/mCellHeight;
  mLock.unlock();
}

void Helper::unselectCell()
{
    mLock.lock();
    mSelectedPosition = false;
    mLock.unlock();
}

bool Helper::isSelected()
{
    return mSelectedPosition;
}

int Helper::getSelectedRow()
{
    return mSelectedRow;
}

int Helper::getSelectedColumn()
{
    return mSelectedColumn;
}
