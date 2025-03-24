# approachObject

## General description
This module has been designed to perform the final movement of the robot at the end of a successful search.
When the search object is found, this module brings the robot closer to it, so that it can grab it or point it more accurately with its hand.

## Usage:
The module expects as input the name of the object followed by a Bottle with its coordinates. You can pass both the pixel coordinates of the object with respect to the camera reference frame (Bottle size = 2) or the absolute location of the object (Bottle size = 3);
#### Example with pixel coordinates from the camera
Input format:   `<objectName> (<coordsX> <coordsY>)`
Example:        `ball (156 203)`
#### Example with absolute location of the target object
Input format:   `<objectName> (<coordsX> <coordsY> <coordsZ>)`
Example:        `tv (-4.0 -1.0 0.9)`

These coordinates define a point in space to which the robot approaches keeping a safe distance (editable in the .ini file).
Once approached to the object, the robot searches for the object again and returns its pixel coordinates in an output port.

You should use this module:
- using as input what is returned by the positive outcome port of the r1Obr-orchestrator module
- connecting the output to the cer handPointing module

