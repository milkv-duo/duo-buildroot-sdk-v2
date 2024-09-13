import numpy as np
import sys
import cv2

def decode_binary(binf,imgf):
    img = cv2.imread(imgf)[:,:,::-1]
    img = img.transpose([2,0,1]).astype(np.float32)*254.998/255 #qscale
    img = img.clip(0,255).astype(np.float32)
    c,h,w = img.shape

    binimg = np.fromfile(binf,dtype=np.uint8).reshape(c,h,w).astype(np.float32)

    diff = np.abs(img-binimg)

    print('meandiff:',np.mean(diff))

def decode_output(binf):
    # data = np.fromfile(binf,dtype=np.uint8)
    data = np.load(binf)
    # print('data:',data/255.0)
    print('data:',data.shape,data.dtype)
    data.tofile(binf.replace('.npy','.bin'))
if __name__ == "__main__":
    # decode_binary(sys.argv[1],sys.argv[2])
    decode_output(sys.argv[1])

