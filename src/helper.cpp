#include "helper.h"

#include <QPainter>
#include <QPaintEvent>
#include <QWidget>
#include <QDebug>

Helper::Helper(Animation* animation):
    mAnimation(animation),
    mBackgroundBrush(QColor(255, 255, 255)),
    mForegroundBrush(QColor(0, 0, 0)),
    mBoldPen(Qt::black),
    mTextPen(Qt::white),
    mTextFont("Arial", 12, QFont::Bold),
    mIsPlaying(false),
    mTabIndexBeforePlay(0),
    mSelectedCell(QPoint(0,0)),
    mFirstFramePlay(false),
    mIsSelected(false),
    mCellSize(QSize(0,0)),
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

    int cellWidth = width / mAnimation->getSize().height();
    int cellHeight = height / mAnimation->getSize().width();
    mCellSize = QSize(cellWidth, cellHeight);
    for (int i = 1; i < mAnimation->getSize().height(); i++)
    {
        painter->drawLine(i * mCellSize.width(), 0, i * mCellSize.width(), height);
    }

    for (int i = 1; i < mAnimation->getSize().width(); i++)
    {
        painter->drawLine(0,i*mCellSize.height(),width,i*mCellSize.height());
    }
}

void Helper::drawColors(QPainter *painter)
{
    for (int i = 0; i < mAnimation->getSize().width(); i++)
    {
        for (int j = 0; j < mAnimation->getSize().height(); j++)
        {
            if (mAnimation->getColor(QPoint(i,j)) != Qt::white)
            {
                mForegroundBrush = QBrush(mAnimation->getColor(QPoint(i,j)));
                const QRect rect = QRect(j*mCellSize.width()+2, i*mCellSize.height()+2, mCellSize.width()-4, mCellSize.height()-4);
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

    if (mIsSelected) //clicked
    {
        painter->setPen(mBoldPen);
        painter->drawRect(mSelectedCell.y() * mCellSize.width(),
                          mSelectedCell.x() * mCellSize.height(),
                          mCellSize.width(), mCellSize.height());
    }

    if (mIsPlaying)
    {
        mAnimation->nextFrame();
    }

    mLock.unlock();
}

void Helper::selectCell(const QPoint &pos)
{
  mLock.lock();
  mIsSelected = true;
  mSelectedCell.setY(pos.x() / mCellSize.width());
  mSelectedCell.setX(pos.y()/mCellSize.height());
  qDebug() << "Mouse pressed: " << pos.x() << " " << pos.y();
  qDebug() << "Selected cell: " << mSelectedCell << "\n";

  mLock.unlock();
}

void Helper::unselectCell()
{
    mLock.lock();
    mIsSelected = false;
    mLock.unlock();
}

bool Helper::isSelected()
{
    return mIsSelected;
}

QPoint Helper::getSelectedCell() const
{
	return mSelectedCell;
}
