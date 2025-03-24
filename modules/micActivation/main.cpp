#include <Module.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/ResourceFinder.h>

int main(int argc, char** argv)
{
    yarp::os::ResourceFinder rf;
    rf.configure(argc,argv);

    Module module;
    if(module.runModule(rf))
    {
        yError() << "Module not running";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
