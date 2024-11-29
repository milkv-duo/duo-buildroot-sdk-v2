import numpy as np
import cv2

def show_rgb_image(title, rgb):
    cv2.namedWindow(title, cv2.WINDOW_NORMAL)
    cv2.imshow(title, rgb)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
def show_yuv_image(title, yuv):
    rgb = cv2.cvtColor(yuv, cv2.COLOR_YCrCb2BGR)
    show_rgb_image(title, rgb)

class LoadYUV:
    def __init__(self, filename, size):
        self.width, self.height = size
        self.frame_len = int(self.width * self.height * 3 / 2)
        self.f = open(filename, 'rb')
        self.shape = (int(self.height*1.5), self.width)

    def read_raw(self):
        try:
            raw = self.f.read(self.frame_len)
            yuv = np.frombuffer(raw, dtype=np.uint8)
            yuv = yuv.reshape(self.shape)
        except Exception as e:
            return False, None
        return True, yuv

    def read(self):
        ret, yuv = self.read_raw()
        if not ret:
            return ret, yuv
        bgr = cv2.cvtColor(yuv, cv2.COLOR_YUV420p2RGB)
        return ret, bgr
