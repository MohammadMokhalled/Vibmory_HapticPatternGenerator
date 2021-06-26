#ifndef PLAYINGANIMATION_H
#define PLAYINGANIMATION_H

#include <QObject>
#include <QRunnable>
#include <QThread>
#include "projectsetting.h"

class PlayingAnimation : public QThread
{

public:
    explicit PlayingAnimation(ProjectSetting * prj = nullptr);
    void run() override;

private:
    ProjectSetting * prj;

};

#endif // PLAYINGANIMATION_H
