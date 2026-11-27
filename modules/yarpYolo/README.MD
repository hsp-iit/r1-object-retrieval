# YARP YOLO

Object detection on a YARP image stream with [Ultralytics YOLO](https://docs.ultralytics.com).

## Requirements
- [ultralytics](https://github.com/ultralytics/ultralytics) 8.4 (it brings PyTorch, OpenCV and numpy)
- the Python bindings of YARP 4.0

[environment.yml](environment.yml) creates a conda environment with them, except the YARP bindings
(conda-forge has no YARP 4.0 build), which come from your YARP build:
```bash
conda env create -f modules/yarpYolo/environment.yml
conda env config vars set -n yarpyolo PYTHONPATH=<yarp build or install>/lib/python3
conda activate yarpyolo
```
[environment_cpu.yml](environment_cpu.yml) is the same environment (named `yarpyolo-cpu`) with the CPU-only
builds of PyTorch: no CUDA libraries, much smaller, but inference runs on the CPU. A CUDA toolkit installed
on the system is not used by PyTorch either way: its GPU builds ship their own CUDA runtime and only need
the NVIDIA driver.

The Python version of the environment (3.12) must be the one the YARP bindings were built for:
edit `environment.yml` if it differs. The variable replaces `PYTHONPATH` while the environment is active.
The yarpmanager applications run `python3 yarpYolo.py`: start the `yarprun` of the node from the
activated environment to use it.

## Model
The model is a file found by the YARP ResourceFinder in the context `yolo`, for example
`~/.local/share/yarp/contexts/yolo/yolo26l.pt` (`~/.local/share/yarp` is the default `YARP_DATA_HOME`).
To download the default model there:
```bash
mkdir -p ~/.local/share/yarp/contexts/yolo
python3 -c "from ultralytics.utils.downloads import attempt_download_asset as d; d('$HOME/.local/share/yarp/contexts/yolo/yolo26l.pt')"
```
Any other Ultralytics detection model (e.g. `yolo26s.pt`, or a custom trained one) can be used with the `model`
parameter: the labels accepted by the commands below are the class names of the model.

## Parameters
| Parameter          | Default                     | Description |
|--------------------|-----------------------------|-------------|
| `model`            | `yolo26l.pt`                | model file name |
| `model_context`    | `yolo`                      | ResourceFinder context where the model is searched |
| `device`           | `0` if a GPU is available, else `cpu` | inference device (`cpu` or CUDA device index) |
| `min_confidence`   | `0.95`                      | detections below this confidence are discarded |
| `where_timeout`    | `5.0`                       | seconds the `where` command waits for a new image |
| `image_width`, `image_height` | `640`, `480`     | size of the input images |
| `period`           | `0.035`                     | module period [s] |
| `command_port`, `input_image_port`, `output_image_port`, `where_coord_port` | see below | port names |

## Usage
When running the Python script, four ports will be created:
- the input image port (default `/yarpYolo/image:i`)
- the input command RPC port (default `/yarpYolo/command/rpc`)
- the output image port, where the result of the inference is plotted (default `/yarpYolo/image:o`)
- the output bottle port, where some information of all the objects detected are streamed (default `/yarpYolo/where_coords:o`),
  as a list of `(<label> <confidence> <center x> <center y>)`, or `nothing`

The commands which can be sent to the RPC port are:
- `detect`: restore the default situation, detecting all the objects in the input image
- `label <something>`: highlights all the occurrencies of "something" in the input image
- `where <something>`: returns if "something" is found in the next processed image
