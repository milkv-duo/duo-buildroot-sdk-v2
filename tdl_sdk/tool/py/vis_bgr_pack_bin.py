import numpy as np

import cv2
import sys
import json

def decode_rgb_pack(binf, s=1):
    data = np.fromfile(binf, dtype=np.uint8)
    print('datalen:', data.shape)
    datawh = data[:8].view('int32')

    w,h = datawh[0],datawh[1]
    img = data[8:]
    print('imgshape:', img.shape)
    img = img.reshape(datawh[1], -1,3).astype(np.uint8)
    img = img[:,:w,:]
    print(img.shape)
    print('datawh:', datawh)
    # rgb_pack = np.transpose(img, [1, 2, 0])
    # bgr_pack = rgb_pack[:, :, ::-1].copy()
    
    return img


if __name__ == "__main__":
    img = decode_rgb_pack(sys.argv[1])
    cv2.imshow("img",img)
    cv2.waitKey(0)