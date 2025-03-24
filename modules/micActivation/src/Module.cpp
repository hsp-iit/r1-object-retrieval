#include <Module.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Value.h>
#include <yarp/os/Property.h>

bool Module::configure(yarp::os::ResourceFinder &rf)
{
    /* Configure joystick input port and microphone driver */

    std::string joystick_port_name = rf.check("joystick_port",yarp::os::Value("/micActivation/joystick:i")).asString();
    if(!joystick_port_.open(joystick_port_name))
    {
        yError() << "Cannot open joystick port";
    }

    yarp::os::Property prop;
    prop.put("device","audioRecorder_nwc_yarp");
    prop.put("local","/micActivation");
    prop.put("remote","/audioRecorder_nws");

    if(!drv_.open(prop))
    {
        yError() << "Cannot open device";
        return false;
    }

    if(!drv_.view(iAudioGrabber_))
    {
        yError() << "Cannot set IAudioGrabberInterface";
        return false;
    }

    if(iAudioGrabber_->isRecording(is_microphone_open_))
    {
        iAudioGrabber_->resetRecordingAudioBuffer();
        iAudioGrabber_->stopRecording();
    }

    return true;
}

bool Module::updateModule()
{
    if(auto button_pressed = joystick_port_.read())
    {

        if(!iAudioGrabber_)
        {
            yError() << "Interface not defined, aborting";
            return false;
        }

        iAudioGrabber_->isRecording(is_microphone_open_);

        if (button_pressed->get(0).asInt8() != 0 || button_pressed->get(1).asInt8() != 0)
        {
            /* Start recording if microphone is stopped */
            if(!is_microphone_open_)
            {
                yInfo() << "Started recording";
                iAudioGrabber_->startRecording();
            }
        }

        if (button_pressed->get(0).asInt8() == 0 && button_pressed->get(1).asInt8() == 0)
        {
            if(is_microphone_open_)
            {
                yInfo() << "Stopped recording";
                iAudioGrabber_->stopRecording();
            }
        }
    }

    return true;
}


bool Module::interruptModule()
{
    joystick_port_.interrupt();
    drv_.close();
    return true;
}

bool Module::close()
{
    joystick_port_.close();
    drv_.close();
    return true;
}

double Module::getPeriod()
{
    return 0.01;
}
