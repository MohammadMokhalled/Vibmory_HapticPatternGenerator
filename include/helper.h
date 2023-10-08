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

#ifndef HELPER_H
#define HELPER_H

#include <QBrush>
#include <QFont>
#include <QPen>
#include <QWidget>
#include <frame.h>
#include <animation.h>
#include <QMutex>
#include <QAudioOutput>
#include <QFile>

/**
 * This is a helper class that provides some helper functions for animation drawing and audio playing.
 */
class Helper 
{
public:
    /**
     * Constructor for the Helper class.
     * 
     * @param animation The animation object.
     * @param rows The number of rows.
     * @param columns The number of columns.
     */
    Helper(Animation* animation);

    /**
     * This function is responsible for painting on the QPainter object.
     * 
     * @param painter The QPainter object to paint on.
     * @param event The QPaintEvent object representing the event that triggered the painting.
     */
    void paint(QPainter *painter, QPaintEvent *event);


    /**
     * Draws the background using the specified QPainter object.
     * 
     * @param painter - The QPainter object used for drawing.
     */
    void drawBackground(QPainter *painter);
    
    /**
     * Selects the cell at the specified coordinates.
     * 
     * @param x - The x-coordinate of the cell.
     * @param y - The y-coordinate of the cell.
     */
    void selectCell(const QPoint& pos);
    
    /**
     * Draws the colors cells.
     * 
     * @param painter - The QPainter object used for drawing.
     */
    void drawColors(QPainter * painter);

    /**
     * Unselects the current cell.
     */
    void unselectCell();

    /**
     * Checks if a cell is currently selected.
     * @return true if a cell is selected, false otherwise.
     */
    bool isSelected();
    
    /**
     * Gets the selected cell coordination.
     * @return The selected cell coordination.
     */
    QPoint getSelectedCell() const;

    /**
     * Start playing animation.
     */
    void startPlay();
    
    /**
     * Stop playing animation.
     */
    void stopPlay();


private:
    Animation* mAnimation;
    QBrush mBackgroundBrush;
    QBrush mForegroundBrush;
    QPen mBoldPen;
    QPen mTextPen;
    QFont mTextFont;
    bool mIsPlaying;
    qint32 mTabIndexBeforePlay;
    QPoint mSelectedCell;
    bool mIsSelected;

    QSize mCellSize;
    QMutex mLock;
    
};

#endif
