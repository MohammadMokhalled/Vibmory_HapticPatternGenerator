#ifndef PLAYINGANIMATION_H
#define PLAYINGANIMATION_H

#include <QObject>
#include <QRunnable>
#include <QThread>
#include "projectsetting.h"

class PlayingAnimation : public QThread
{

public:
    explicit PlayingAnimation(projectsetting * prj = nullptr);
    void run() override;

private:
    projectsetting * prj;

};

#endif // PLAYINGANIMATION_H
