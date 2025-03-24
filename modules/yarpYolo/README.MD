# YARP YOLO

## Requirements
- opencv
- ultralytics requirements

## Usage
When running the Python script, four ports will be created:
- the input image port
- the input command RPC port
- the output image port, where the result of the inference is plotted
- the output bottle port, where some information of all the objects detected are streamed

The commands which can be sent to the RPC port are:
- `detect`: restore the default situation, detecting all the objects in the input image
- `label <something>`: highlights all the occurrencies of "something" in the input image
- `where <something>`: returns if "something" is found in the input image