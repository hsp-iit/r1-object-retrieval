# lookForObject

## General description
With this module we can give the robot the name of an object to find and it will move its head and turn looking for it.

By default the robot will move its head around nine pre-defined orientations, in order:
_ straight ahead
_ left
_ right
_ up
_ up left
_ up right
_ down
_ down left
_ down right

Otherwise, the head positions can be defined:
- manually, setting the orientation of the head (pitch and yaw) in the HEAD_POSITIONS group of the .ini file 
- automatically, optimizing the head orientations considering the horizonatal and vertical fov of the camera

The module opens a RGBDCamera client, a navigation client and a Remote Control Board client which give access to the necessary methods.

One input port receives the string with the name of the object to find (`/lookForObject/object:i`), and an output port returns if the object is found or not (`/lookForObject/out:o`)`

### robotOrient
The management of the head orientations is delegated to the robotOrient component.
When the `next` function is called, the first 'unchecked' orientation is returned and it's sent to the gaze-controller module.
When all the orientations of the head have been 'checked', the robot turns to inspect the location where it is from another angle.

## Usage:
In order for this module to work correctly, you'll need:
- map, localization and position NWS
- RGBDCamera NWS
- cer-gaze-controller module
- an object-recognition module

The input port reads the name of the object to search and the result of the search is returned to the output port.
If you want to stop the robot during the search for an object, you can just send a "stop" command to the input port.


