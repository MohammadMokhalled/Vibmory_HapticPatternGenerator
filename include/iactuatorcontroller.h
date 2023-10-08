#ifndef I_ACTUATOR_CONTROLLER_H
#define I_ACTUATOR_CONTROLLER_H

#include <QtGlobal>
#include "animation.h"

/**
 * @brief The IActuatorControl class
 * This class is the interface for the actuator controls like controlling by audio.
 */
class IActuatorController
{
public:
	/**
	 * Constructs an IActuatorControl object with the specified animation.
	 * @param animation The Animation object.
	 */
    IActuatorController() = default;

    /**
     * Set the frame rate of the animation.
     * @param frameRate The frame rate of the animation.
     */
    virtual void setFrameRate(qint32 frameRate) = 0;

    /**
     * Set the duration of playing in seconds.
     * @param duration The duration of playing in seconds.
     */
    virtual void setDuration(qint32 duration) = 0;

    /**
	 * prepare the object for controlling the actuators.
	 * @return True if the object is successfully prepared, false otherwise.
	 */
	virtual bool prepare() = 0;

	/**
	 * starts controlling the actuators.
	 * @return True if the file is successfully played, false otherwise.
	 */
	virtual bool play()	= 0;

	/**
	 * Stops controlling the actuators.
	 * @return True if the file is successfully stopped, false otherwise.
	 */
	virtual bool stop()	= 0;
};

#endif // ANIMATIONAUDIO_H
