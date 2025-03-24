#include <yarp/os/RFModule.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IAudioGrabberSound.h>

class Module: public yarp::os::RFModule
{
    public:
        bool configure(yarp::os::ResourceFinder& rf) override;
        bool updateModule() override;
        bool interruptModule() override;
        bool close() override;
        double getPeriod() override;

    private:
        yarp::os::BufferedPort<yarp::os::Bottle> joystick_port_;
        yarp::dev::PolyDriver drv_;
        yarp::dev::IAudioGrabberSound* iAudioGrabber_;
        bool is_microphone_open_;
};
