# YARP MDETR

## Requirements
- opencv
- see mdetr_python_code/requirements.txt

## Usage
When running the Python script, four ports will be created:
- the input image port
- the input command RPC port
- the output image port, where the result of the inference is plotted
- the output bottle port, where some information of the searched objects are streamed, if detected

The commands which can be sent to the RPC port are:
- `label <something>`: detect all the occurrencies of "something" in the input image and the corresponding bounding boxes are showed on the output image
- `where <something>`: detect all the occurrencies of "something" in the input image and the corresponding bounding boxes are showed on the output image, returning if "something" is found in the input image