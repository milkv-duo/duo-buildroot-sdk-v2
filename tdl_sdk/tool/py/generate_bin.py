import numpy as np

import cv2
import sys
import glob
import os
def convert_image_to_bin(src_file,dst_file,plannar):
    img = cv2.imread(src_file)
    h,w,_ = img.shape
    wh = np.array([w,h]).astype(np.int32)
    if plannar:
        img = np.transpose(img,[2,0,1])
        assert(img.shape[0] == 3)

    print('w:',w,',h:',h)
    # for i in range(100):
    #     print(img[0,0,i])
    dstf = open(dst_file,'wb')
    dstf.write(wh.tobytes())
    dstf.write(img.tobytes())

    dstf.close()

def convert_img_folder(src_dir,dst_dir):
    src_imgs = sorted(glob.glob(src_dir+'/*.jpg'))
    os.makedirs(dst_dir,exist_ok=True)
    for idx,img in enumerate(src_imgs):
        name = os.path.split(img)[1].split('.')[0]+'.bin'
        dstf = dst_dir+'/'+name
        convert_image_to_bin(img,dstf,True)
        if idx>10:
            break

if __name__ == "__main__":
    convert_img_folder(sys.argv[1],sys.argv[2])



