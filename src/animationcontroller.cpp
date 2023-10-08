#include "animationcontroller.h"

AnimationController::AnimationController(const QString &fileName,
                                         QObject *parent)
    :
    QObject(parent),
    mAnimation(new Animation(fileName)),
    mAudioActuator(new AudioActuator(mAnimation)),
      mHelper(new Helper(mAnimation)),
    mPaintingWidget(new PaintingWidget(mHelper)),
    mTimer(new QTimer(this)),
    mStopTimer(new QTimer(this))
{
    connect(mPaintingWidget, &PaintingWidget::selectedSignal, this,
        			&AnimationController::selectedSignal);
    connect(mTimer, &QTimer::timeout, mPaintingWidget, &PaintingWidget::animate);
    mTimer->start(50);
}

AnimationController::AnimationController(const QSize &size, QObject *parent)
    :
    QObject(parent),
    mAnimation(new Animation(size)),
    mAudioActuator(new AudioActuator(mAnimation)),
      mHelper(new Helper(mAnimation)),
    mPaintingWidget(new PaintingWidget(mHelper)),
    mTimer(new QTimer(this)),
    mStopTimer(new QTimer(this))
{
    connect(mPaintingWidget, &PaintingWidget::selectedSignal, this,
            &AnimationController::selectedSignal);
    connect(mTimer, &QTimer::timeout, mPaintingWidget,
            &PaintingWidget::animate);
    mTimer->start(50);
}

AnimationController::~AnimationController()
{
}

QSize AnimationController::getSize() const
{
	return mAnimation->getSize();
}

int AnimationController::getLen() const
{
    return mAnimation->getLen();
}

void AnimationController::addFrame()
{
    mAnimation->addFrame();
}

void AnimationController::duplicateCurrentFrame()
{
    mAnimation->duplicateCurrentFrame();
}

void AnimationController::removeFrame()
{
    mAnimation->removeCurrentFrame();
}

void AnimationController::selectFrame(int index)
{
    mAnimation->selectFrame(index);
}

QWidget* AnimationController::getPaintingWidget()
{
    return mPaintingWidget;
}

void AnimationController::drawBackground()
{
    mPaintingWidget->drawBackground();
}

void AnimationController::playAnimation(quint16 framerate, quint32 duration)
{
    mTimer->stop();
    mTimer->start((double)1000/(double)framerate);
    
    connect(mStopTimer, &QTimer::timeout, this, &AnimationController::stopAnimation);
    mStopTimer->start(1000*duration);
    mHelper->startPlay();
    mAudioActuator->play();
}

void AnimationController::stopAnimation()
{
    mHelper->stopPlay();
    mAudioActuator->stop();
    mTimer->stop();
    mTimer->start(50);
    disconnect(mStopTimer, &QTimer::timeout, this,
               &AnimationController::stopAnimation);
    mStopTimer->stop();
    emit stopAnimationSignal();
}

void AnimationController::saveToFile(const QString& fileName)
{
    mAnimation->writeInFile(fileName);
}

void AnimationController::setFrameRate(qint32 frameRate)
{
    mAudioActuator->setFrameRate(frameRate);
}

void AnimationController::setDuration(qint32 duration)
{
    mAudioActuator->setDuration(duration);
}

bool AnimationController::prepareActuator(QString &fileName)
{
    return mAudioActuator->prepare();
}

int AnimationController::getAmplitude(const QPoint& pos) const
{
    if(pos == QPoint(-1, -1))
		return mAnimation->getAmplitude(mHelper->getSelectedCell());
	else
	    return mAnimation->getAmplitude(pos);
}

int AnimationController::getFrequency(const QPoint& pos) const
{
    if(pos == QPoint(-1, -1))
        return mAnimation->getFrequency(mHelper->getSelectedCell());
    else
	    return mAnimation->getFrequency(pos);
}

void AnimationController::setFrequency(quint32 value)
{
    mAnimation->setFrequency(mHelper->getSelectedCell(), value);
}

void AnimationController::setAmplitude(quint32 value)
{
    mAnimation->setAmplitude(mHelper->getSelectedCell(),
                             value);
}

void AnimationController::unselect()
{
    mHelper->unselectCell();
}

bool AnimationController::isSelected() const
{
	return mHelper->isSelected();
}



