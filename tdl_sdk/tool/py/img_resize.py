import cv2
import sys

def resize_img(srcf,dstf,dst_size):
    img = cv2.imread(srcf)
    img = cv2.resize(img,dst_size)
    cv2.imwrite(dstf,img)


if __name__ == "__main__":
    resize_img(sys.argv[1],sys.argv[2],(768,432))