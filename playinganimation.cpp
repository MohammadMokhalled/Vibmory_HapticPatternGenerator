#include "playinganimation.h"
#include "QDebug"
#include <QThreadPool>
#include <QGraphicsScene>

PlayingAnimation::PlayingAnimation(projectsetting * prj)
{
    this->prj = prj;
}

void PlayingAnimation::run()
{
    if(this->prj == nullptr)
    {
        return;
    }

    for (int i = 0; i < 100; i++)
    {
        for (int j = 0; j < prj->rows * prj->columns; j++)
        {
            //QGraphicsScene * scene = new QGraphicsScene()
            //prj->graphicViews->setBackgroundBrush()

        }
    }

}
