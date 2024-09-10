import cv2
import sys
import numpy as np
# from data_vis import decode_rgb_planar

def pytorch_preprocess(img,mean,scale):
    img1 = (img-mean)/scale
    return img1

def check_img_eq(imgf1,imgf2):
    vpss_rgb = decode_rgb_planar(imgf1,is_bgr=True).astype(np.int8)
    print(vpss_rgb[0,0,:])
    img = cv2.imread(imgf2)
    print(img[0,0,:])
    mean = np.array([127.5,127.5,127.5])
    scale = 128

    vpss_quant = 128.251
    pytorch_rgb = pytorch_preprocess(img,mean,scale)
    py_rgb = (pytorch_rgb*vpss_quant).astype(np.int8)
    print('pyrgb:',py_rgb[0,0,:])
    diff = np.abs(vpss_rgb - py_rgb)

    print('maxdiff:',np.max(diff),',mean:',np.mean(diff))

def check_data_eq(binf1,binf2):
    data1 = np.fromfile(binf1,dtype=np.int8)
    data2 = np.fromfile(binf2,dtype=np.int8)

    diff = np.abs(data1-data2)
    print('mean:',np.mean(diff),',var:',np.var(diff),np.min(diff),np.max(diff))
    

if __name__ == "__main__":
    # const_img = np.ones((432,768,3))*100
    # const_img = const_img.astype(np.uint8)
    # cv2.imwrite(sys.argv[1],const_img)
    # sys.exit()
    check_data_eq(sys.argv[1],sys.argv[2])