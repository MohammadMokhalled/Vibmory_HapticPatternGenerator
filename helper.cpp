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
Helper::Helper(Animation * anim, int rows, int columns)
{
    QLinearGradient gradient(QPointF(50, -20), QPointF(80, 20));
    gradient.setColorAt(0.0, Qt::white);
    gradient.setColorAt(1.0, QColor(0xa6, 0xce, 0x39));

    background = QBrush(QColor(255, 255, 255));
    circleBrush = QBrush(gradient);
    circlePen = QPen(Qt::black);
    circlePen.setWidth(1);
    boldPen = QPen(Qt::black);
    boldPen.setWidth(4);
    textPen = QPen(Qt::white);
    textFont.setPixelSize(50);
    this->rows = rows;
    this->columns = columns;
    animation = anim;
    //this->ps = p;
}
//! [0]

//! [1]

void Helper::drawBackground(QPainter *painter)
{
    int width = painter->device()->width();
    int height = painter->device()->height();
    const QRect rect = QRect(QPoint(0,0), QPoint(width,height));
    painter->fillRect(rect, background);

    cellWidth = width / columns;
    cellHeight = height / rows;

    for (int i = 1; i < columns; i++)
    {
        painter->drawLine(i*cellWidth,0,i*cellWidth,height);
    }

    for (int i = 1; i < rows; i++)
    {
        painter->drawLine(0,i*cellHeight,width,i*cellHeight);
    }

}

void Helper::drawColors(QPainter *painter)
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < columns; j++)
        {
            if (animation->getColor(i,j) != Qt::white)
            {
                foreground = QBrush(animation->getColor(i,j));
                const QRect rect = QRect(j*cellWidth+2, i*cellHeight+2, cellWidth-4, cellHeight-4);
                painter->fillRect(rect, foreground);
            }
        }
    }
}

void Helper::playAudio()
{
    sourceFile.setFileName("file.wav"); //C:/Users/mmokh/OneDrive/Documents/build-Sensors_CPP-Desktop_Qt_6_0_3_MSVC2019_64bit-Debug/
    sourceFile.open(QIODevice::ReadOnly);

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(48000);
    format.setChannelCount(animation->getColumns() * animation->getRows());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
    }

    audio = new QAudioOutput(format);
    audio->start(&sourceFile);
    //audio->start();
}

void Helper::startPlay()
{
    tabIndexBeforePlay = animation->getCurrentFrameIndex();
    animation->selectFrame(0);
    play = true;
    firstFramePlay = true;
}

void Helper::stopPlay()
{
    play = false;
    animation->selectFrame(tabIndexBeforePlay);
    audio->stop();
    sourceFile.close();
}


void Helper::paint(QPainter *painter, QPaintEvent *event)
{
    lock.lock();
    if (firstFramePlay)
    {
        playAudio();
        firstFramePlay = false;
    }
    drawBackground(painter);

    drawColors(painter);

    if (selectedPosition) //clicked
    {
        painter->setPen(boldPen);
        painter->drawRect(selectedColumn * cellWidth, selectedRow * cellHeight, cellWidth, cellHeight);
    }

    if (play)
    {
        animation->nextFrame();
    }

    lock.unlock();
}

bool Helper::setPaintingState(int state)
{
  lock.lock();
  this->paintingState = state;
  lock.unlock();
  return true;
}

void Helper::selectCell(int x, int y)
{
  lock.lock();
  selectedPosition = true;
  selectedColumn = x/cellWidth;
  selectedRow = y/cellHeight;
  lock.unlock();
}

void Helper::unselectCell()
{
    lock.lock();
    selectedPosition = false;
    lock.unlock();
}

bool Helper::isSelected()
{
    return selectedPosition;
}

int Helper::getSelectedRow()
{
    return selectedRow;
}

int Helper::getSelectedColumn()
{
    return selectedColumn;
}
