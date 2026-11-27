import sys
import time
import ctypes
import numpy as np
import cv2
import yarp
from ultralytics import YOLO
from ultralytics.utils.plotting import Annotator, colors
import torch

# Initialise YARP
yarp.Network.init()


def as_numpy(img):
    """Writable numpy view (height, width, 3) on the memory of a yarp.ImageRgb.

    yarp.Image.setExternal() cannot be used to share memory with numpy: it crashes
    with the YARP 4.0 Python bindings (they release the GIL before it calls the
    Python buffer API).
    """
    h, w, row = img.height(), img.width(), img.getRowSize()
    buf = (ctypes.c_uint8 * (row * h)).from_address(int(img.getRawImage()))
    return np.ctypeslib.as_array(buf).reshape(h, row)[:, :w * 3].reshape(h, w, 3)

class YarpYolo(yarp.RFModule):
    
    def configure(self, rf):
        
        # Generic configs
        self.period = rf.find('period').asFloat32() if rf.check('period') else 0.035
        
        self.image_w = rf.find('image_width').asInt32() if rf.check('image_width') else 640
        self.image_h = rf.find('image_height').asInt32() if rf.check('image_height') else 480
        self.min_conf = rf.find('min_confidence').asFloat32() if rf.check('min_confidence') else 0.95
        self.where_timeout = rf.find('where_timeout').asFloat32() if rf.check('where_timeout') else 5.0
        # inference device: "cpu", or the CUDA device index (default: first GPU, if any)
        self.device = rf.find('device').toString() if rf.check('device') else ('0' if torch.cuda.is_available() else 'cpu')
        print('Inference device: {:s}'.format(self.device))

        # Opening ports
        self.cmd_port = yarp.Port()
        commandPortName = rf.find('command_port').asString() if rf.check('command_port') else '/yarpYolo/command/rpc' 
        self.cmd_port.open(commandPortName)
        print('{:s} opened'.format(commandPortName))
        self.attach(self.cmd_port)

        self._input_image_port = yarp.BufferedPortImageRgb()
        imageInPortName = rf.find('input_image_port').asString() if rf.check('input_image_port') else '/yarpYolo/image:i'
        self._input_image_port.open(imageInPortName)
        print('{:s} opened'.format(imageInPortName))

        self._output_image_port = yarp.Port()
        imageOutPortName = rf.find('output_image_port').asString() if rf.check('output_image_port') else '/yarpYolo/image:o' 
        self._output_image_port.open(imageOutPortName)
        print('{:s} opened'.format(imageOutPortName))

        self.output_coords_port = yarp.BufferedPortBottle()
        coordsOutPortName = rf.find('where_coord_port').asString() if rf.check('where_coord_port') else '/yarpYolo/where_coords:o' 
        self.output_coords_port.open(coordsOutPortName)
        print('{:s} opened'.format(coordsOutPortName))

        # Labels detected in the last processed frame, and number of processed frames:
        # used to answer the 'where' command
        self._detected = set()
        self._frame_count = 0

        # Preparing images
        self._in_buf_image = yarp.ImageRgb()
        self._in_buf_image.resize(self.image_w, self.image_h)
        self._out_buf_image = yarp.ImageRgb()
        self._out_buf_image.resize(self.image_w, self.image_h)
        self._out_buf_array = as_numpy(self._out_buf_image)

        # Defining model
        self.model = YOLO("yolov8l.pt")
        # self.model = YOLO("yolov8s.pt")
        
        # -1: highlight every detection; 999: the requested label is unknown, highlight nothing
        self.label_num = -1

        # Class names of the model, e.g. COCO for the pretrained models
        self.labels = self.model.names                            # class index -> name
        self.label_inv = {v: k for k, v in self.labels.items()}   # name -> class index

        return True

    def respond(self, command, reply):
        if command.get(0).asString() == 'detect':
            print('Command \'detect\' received')
            self.label_num = -1
        elif command.get(0).asString() == 'label':
            print('Command \'label\' received')
            label = command.get(1).asString()
            if label in self.label_inv:
                self.label_num = self.label_inv[label]
                reply.addString('labeling: ' + str(label))
            else:
                reply.addString('cannot label (not in the available list): ' + str(label))
                self.label_num = 999
        elif command.get(0).asString() == 'where':
            print('Command \'where\' received')
            label = command.get(1).asString()
            if label in self.label_inv:
                self.label_num = self.label_inv[label]
            else:
                self.label_num = 999
                reply.addString('not found')
                reply.addString('cannot find (not in the available list): ' + str(label))
                return True
            # Answer with the first frame processed after the request. Polling with sleep lets the
            # main thread run: a blocking read here would keep the GIL (YARP 4.0 Python bindings)
            # and prevent the main thread from processing the frame.
            frame_count = self._frame_count
            start = time.time()
            while self._frame_count == frame_count and time.time() - start < self.where_timeout:
                time.sleep(0.01)
            if self._frame_count == frame_count:
                print('No image processed in {:.1f} s'.format(self.where_timeout))
                reply.addString('not found')
            elif label in self._detected:
                reply.addString(label + ' found')
            else:
                reply.addString('not found')
                
                
        elif command.get(0).asString() == 'help':
            print('Command \'help\' received')
            reply.addVocab32('many')
            reply.addString('detect : starts detecting everything in the input image')
            reply.addString('label <something> : identify "something" in input image')
            reply.addString('where <something> : returns whether "something" is found in the input image')
            reply.addString('help : get this list')           
        else:
            print('Command {:s} not recognized'.format(command.get(0).asString()) + '. Type \'help\'')
            reply.addString('Command {:s} not recognized'.format(command.get(0).asString()) + '. Type \'help\'')
        
        if reply.size()==0:
            reply.addVocab32('ack')
        return True

    
    def interruptModule(self):
        print('Interrupt function')
        self.cmd_port.close()
        self._input_image_port.close()
        self._output_image_port.close()
        self.output_coords_port.close()
        return True

    
    def getPeriod(self):
        return self.period


    def plot_inference(self, frame):
        # numpy images are expected in BGR order by ultralytics, while YARP images are RGB
        img = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        result = self.model.predict(img, conf=self.min_conf, device=self.device, verbose=False)[0]
        boxes = result.boxes

        annotator = Annotator(frame)
        bout = self.output_coords_port.prepare()
        bout.clear()
        for (x1, y1, x2, y2), conf, cls in zip(boxes.xyxy.tolist(), boxes.conf.tolist(), boxes.cls.int().tolist()):
            label = self.labels[cls]
            if self.label_num >= 0 and cls != self.label_num:
                color = (211, 211, 211)  # light grey
            else:
                color = colors(cls)      # RGB, like the frame
            annotator.box_label((x1, y1, x2, y2), '{:s} {:.1f}%'.format(label, 100 * conf), color)

            b = bout.addList()
            b.addString(label)
            b.addFloat32(conf)
            b.addFloat32((x1 + x2) / 2)
            b.addFloat32((y1 + y2) / 2)
        if len(boxes) == 0:
            bout.addString('nothing')
        self._detected = {bout.get(i).asList().get(0).asString() for i in range(bout.size()) if bout.get(i).isList()}
        self._frame_count += 1
        self.output_coords_port.write()
        self.imageYolo = annotator.result()


    def updateModule(self):
        received_image = self._input_image_port.read()
        self._in_buf_image.copy(received_image)
        frame = as_numpy(self._in_buf_image)
        self.plot_inference(frame)
        self._out_buf_array[:,:] = self.imageYolo
        self._output_image_port.write(self._out_buf_image)
        return True


if __name__ == '__main__':
    
    rf = yarp.ResourceFinder()
    rf.setVerbose(True)
    rf.setDefaultContext("yarpYolo")
    conffile = rf.find("from").asString()
    if not conffile:
        print('Using default conf file')
        rf.setDefaultConfigFile('yarpYolo_R1.ini')
    else:
        rf.setDefaultConfigFile(rf.find("from").asString())

    rf.configure(sys.argv)

    # Run module
    ap = YarpYolo()
    try:
        ap.runModule(rf)
    finally:
        print('Closing yarpYolo...')
        ap.interruptModule()

