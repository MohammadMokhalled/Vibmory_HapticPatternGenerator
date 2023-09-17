/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "helper.h"

#include <QPainter>
#include <QPaintEvent>
#include <QWidget>
#include <QDebug>

//! [0]
Helper::Helper(Animation* animation, int rows, int columns):
    mAnimation(animation),
    mRows(rows),
    mColumns(columns)
{
    QLinearGradient gradient(QPointF(50, -20), QPointF(80, 20));
    gradient.setColorAt(0.0, Qt::white);
    gradient.setColorAt(1.0, QColor(0xa6, 0xce, 0x39));

    mBackgroundBrush = QBrush(QColor(255, 255, 255));
    mCircleBrush = QBrush(gradient);
    mCirclePen = QPen(Qt::black);
    mCirclePen.setWidth(1);
    mBoldPen = QPen(Qt::black);
    mBoldPen.setWidth(4);
    mTextPen = QPen(Qt::white);
    mTextFont.setPixelSize(50);
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
            if (mAnimation->getColor(i,j) != Qt::white)
            {
                mForegroundBrush = QBrush(mAnimation->getColor(i,j));
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
    format.setChannelCount(mAnimation->getColumns() * mAnimation->getRows());
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
    mPlay = true;
    mFirstFramePlay = true;
}

void Helper::stopPlay()
{
    mPlay = false;
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

    if (mPlay)
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
