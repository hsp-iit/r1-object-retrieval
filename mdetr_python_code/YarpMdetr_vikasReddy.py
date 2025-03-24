import cv2
import numpy as np
import torch
from PIL import Image
import torchvision.transforms as T
from collections import defaultdict
import numpy as np
import cv2
torch.set_grad_enabled(False)
import yarp
from mdetr_python_code.models.model import mdetr_resnet101


# Initialise YARP
yarp.Network.init()

class YarpMdetr(yarp.RFModule):
    def configure(self, rf):

        self.image_w = 640
        self.image_h = 480
        self.module_name = 'YarpMdetr'

        self.cmd_port = yarp.Port()
        self.cmd_port.open('/' + self.module_name + '/command:i')
        print('{:s} opened'.format('/' + self.module_name + '/command:i'))
        self.attach(self.cmd_port)

        self._input_image_port = yarp.BufferedPortImageRgb()
        self._input_image_port.open('/' + self.module_name + '/image:i')
        print('{:s} opened'.format('/' + self.module_name + '/image:i'))


        self._output_image_port = yarp.Port()
        self._output_image_port.open('/' + self.module_name + '/image:o')
        print('{:s} opened'.format('/' + self.module_name + '/image:o'))


        print('Preparing input image...')
        self._in_buf_array = np.ones((self.image_h, self.image_w, 3), dtype=np.uint8)
        self._in_buf_image = yarp.ImageRgb()
        self._in_buf_image.resize(self.image_w, self.image_h)
        self._in_buf_image.setExternal(self._in_buf_array, self._in_buf_array.shape[1], self._in_buf_array.shape[0])

        print('Preparing output image...\n')
        self._out_buf_image = yarp.ImageRgb()
        self._out_buf_image.resize(self.image_w, self.image_h)
        self._out_buf_array = np.zeros((self.image_h, self.image_w, 3), dtype=np.uint8)
        self._out_buf_image.setExternal(self._out_buf_array, self._out_buf_array.shape[1], self._out_buf_array.shape[0])
  
        self.model, postprocessors = torch.hub.load('ashkamath/mdetr:main', 'mdetr_resnet101', pretrained=True, return_postprocessor=True)
        self.postprocessors = postprocessors
        self.model.cuda()
        self.model.eval()
        self.caption = ""
        return True


    def respond(self, command, reply):
        if command.get(0).asString() == 'quit':
            print('Command quit received')
            reply.addString('quit state activated')
        elif command.get(0).asString() == 'caption':
            # print('Command caption received')
            self.caption = command.get(1).asString()
            reply.addString('caption state activated ' + self.caption)
        else:
            print('Command {:s} not recognized'.format(command.get(0).asString()))
            reply.addString('Command {:s} not recognized'.format(command.get(0).asString()))
        return True

    def cleanup(self):
        print('Cleanup function')
        self.cmd_port.close()
        self._input_image_port.close()
        self._output_image_port.close()


    def interruptModule(self):
        print('Interrupt function')
        self.cmd_port.interrupt()
        self._input_image_port.interrupt()
        self._output_image_port.close()
        return True

    def getPeriod(self):
        return 0.001

    def transform(self, im):   
        transform = T.Compose([
            T.ToTensor(),
            T.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),])
        return transform(im)


    def box_cxcywh_to_xyxy(self, x):
        x_c, y_c, w, h = x.unbind(-1)
        b = [(x_c - 0.5 * w), (y_c - 0.5 * h),
             (x_c + 0.5 * w), (y_c + 0.5 * h)]
        return torch.stack(b, dim=-1)
    
    def rescale_bboxes(self, out_bbox, size):
        img_w, img_h = size
        b = self.box_cxcywh_to_xyxy(out_bbox)
        b = b * torch.tensor([img_w, img_h, img_w, img_h], dtype=torch.float32)
        return b
    
    def COLORS(self):
        return np.random.uniform(0, 255, size=(80, 3))
    
    def apply_mask(self, image, mask, color, alpha=0.5):
        """Apply the given mask to the image.
        """
        for c in range(3):
            image[:, :, c] = np.where(mask == 1,
                                      image[:, :, c] *
                                      (1 - alpha) + alpha * color[c] * 255,
                                      image[:, :, c])
        return image
    
    def plot_results_yarp(self ,pil_img, scores, boxes, labels,masks=None):
        self.np_image = np.array(pil_img)
        for s, (xmin, ymin, xmax, ymax), l in zip(scores, boxes.tolist(), labels):
            xmin, ymin, xmax, ymax = map(int, [xmin, ymin, xmax, ymax])
            cv2.rectangle(self.np_image, (xmin, ymin), (xmax, ymax), (0, 255, 0), 2)
            cv2.putText(self.np_image, l, (xmin, ymin - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
        return np.array(pil_img)


    def plot_inference(self, im, caption):
        im = Image.fromarray(im)
        img = self.transform(im).unsqueeze(0).cuda()
        memory_cache = self.model(img, [caption], encode_and_save=True)
        outputs = self.model(img, [caption], encode_and_save=False, memory_cache=memory_cache)
        probas = 1 - outputs['pred_logits'].softmax(-1)[0, :, -1].cpu()
        keep = probas > 0.7
        bboxes_scaled = self.rescale_bboxes(outputs['pred_boxes'].cpu()[0, keep], im.size)
        positive_tokens = (outputs["pred_logits"].cpu()[0, keep].softmax(-1) > 0.1).nonzero().tolist()
        predicted_spans = defaultdict(str)
        for tok in positive_tokens:
            item, pos = tok
            if pos < 255:
                span = memory_cache["tokenized"].token_to_chars(0, pos)
                predicted_spans[item] += " " + caption[span.start:span.end]
        labels = [predicted_spans[k] for k in sorted(list(predicted_spans.keys()))]
        self.plot_results_yarp(im, probas[keep], bboxes_scaled, labels, masks=None)



    def updateModule(self):
    # existing code
        received_image = self._input_image_port.read()
        self._in_buf_image.copy(received_image)   
        assert self._in_buf_array.__array_interface__['data'][0] == self._in_buf_image.getRawImage().__int__()
        frame = self._in_buf_array
        print(self.caption)
        self.plot_inference(frame, self.caption)
        self._out_buf_array[:,:] = self.np_image
        self._output_image_port.write(self._out_buf_image)
        return True





if __name__ == '__main__':

    rf = yarp.ResourceFinder()
    rf.setVerbose(True)
    rf.setDefaultContext("AnnotationsPropagatorModule")

    ap = YarpMdetr()
    # try:
    ap.runModule(rf)
    # finally:
    #     print('Closing SegmentationDrawer due to an error..')
    #     player.cleanup()



