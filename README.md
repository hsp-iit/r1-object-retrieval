# r1-Object-Retrieval

Repository to manage the work on the object retrieval with the robot R1.
   
## Requirements
* [yarp](https://github.com/robotology/yarp)
* [icub-main](https://github.com/robotology/icub-main.git)
* [icub-contrib](https://github.com/robotology/icub-contrib-common.git)
* [cer](https://github.com/robotology/cer)
* [yarp-devices-llm](https://github.com/robotology/yarp-devices-llm)
* [tour-guide-robot](https://github.com/hsp-iit/tour-guide-robot)
* [Mdetr](https://github.com/ashkamath/mdetr/tree/main) requirements
* [Yolo v8](https://github.com/ultralytics/ultralytics) requirements

## Installation
### Using Docker
Build your docker container using the provided Dockerfiles in the [docker](docker) folder, or just pull the images:
```bash
docker pull colombraf/r1images:r1ObjectRetrieval      # to work with the actual R1 robot
docker pull colombraf/r1images:r1ObjectRetrievalSim   # to work in a simulated environment
```

### From source
* On Linux:
```bash
git clone https://github.com/hsp-iit/r1-object-retrieval 
cd r1-object-retrieval && mkdir build && cd build && cmake .. && make -j11
export PYTHONPATH=$PYTHONPATH:${R1_OBR_SOURCE_DIR} 
export YARP_DATA_DIRS=$YARP_DATA_DIRS:${R1_OBR_BUILD_DIR}/share/R1_OBJECT_RETRIEVAL
export PATH=$PATH:${R1_OBR_BUILD_DIR}/bin
```

## Usage
### Simulation
* `cd docker/docker_r1ObjectRetrievalSim`
* `./start_docker_obj_retr_sim.sh`
* Inside the docker container: `./start_sim.sh` . This will launch a yarpserver instance, the gazebo simulation, a yarprun instance (/console), yarplogger and yarpmanager.

### Real robot
* `cd docker/docker_r1ObjectRetrieval`
* `./start_docker_obj_retr.sh`
* Inside the docker container: `yarp run --server /console --log`, `yarplogger --start`, `yarpmanager`

## Modules
You can find here below a list of the main [modules](modules) in this repo and a brief description of their purpose

| Module                                                | Description                                                           |
| ----------------------------------------------------- | --------------------------------------------------------------------- |
| [nextLocPlanner](modules/nextLocPlanner/README.md) | Planner that define the next location the robot should inspect        |
| [lookForObject](modules/lookForObject/README.md) | Module that manages the motions the robot should perfom once arrived in a location to search for the requested object (tilt head and turn)        |
| [goAndFindIt](modules/goAndFindIt/README.md) | FSM that invokes the two previous modules to navigate the robot in the map and search for objects |
| [approachObject](modules/approachObject/README.md) | Plug-in module for the search positive outcome: the robot navigates nearer to the found object  |
| [disappointedPose](modules/disappointmentPose/README.md) | Plug-in module for the search negative outcome: the robot assumes a predefined pose  |
| [yarpMdetr](modules/yarpMdetr/README.MD) / [yarpYolo](modules/yarpYolo/README.MD) | Object Detection modules: detect objects in an input image |
| [r1Obr-orchestrator](modules/r1Obr-orchestrator/README.md) | Orchestrator (FSM) of the whole object retrieval application: manages the search, the speech interaction, the integration of the sensor network, and other possible actions that the robot could perform|
| [sensorNetworkReceiver](modules/sensorNetworkReceiver/README.md) | Interaction module between the r1Obr-orchestrator and the Sensor Network |
| [look_and_point](modules/look_and_point/README.md) | Just a simple module to use the positive output of the search and give it as an iput to the handPointing module (cer repo) |
| [micActivation](modules/micActivation/)| Module that starts/stops the audioRecorder device when a joystick button is pressed/released |


## Interfaces
You can find in the [interfaces](interfaces) folder the thrift files for the creation of the following classes:
* [r1OrchestratorRPC](interfaces/r1OrchestratorRPC/): implementations of the RPC calls of the module r1Obr-orchestrator

## Apps
In the [app](app) folder you can find, divided by context:
* a `scripts` folder containing the definition of the yarp applications, and eventually other useful scripts, for the corresponding module
* a `conf` folder containing the configuration files for the corresponding module