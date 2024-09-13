import json
import os
import sys
from tqdm import tqdm
import cv2

def draw_res(img_root, draw_dir):
    file_list = os.listdir(img_root)
    os.makedirs(draw_dir, exist_ok=True)
    for img_name in tqdm(file_list):
        split_list = img_name.split('_')
        return_list = []
        for i, name in enumerate(split_list):
            if name == 'x1':
                x1 = split_list[i+1]
            elif name == 'y1':
                y1 = split_list[i+1]
            elif name == 'x2':
                x2 = split_list[i+1]
            elif name == 'y2':
                y2 = split_list[i+1]
        box = [int(x) for x in [x1, y1, x2, y2]]
        img_path = os.path.join(img_root, img_name)
        img = cv2.imread(img_path)
        img = cv2.rectangle(img, (box[0], box[1]), (box[2], box[3]), color=(0, 0, 255), thickness=2)
        cv2.imwrite(os.path.join(draw_dir, img_name), img)

if __name__ == "__main__":
    draw_res(sys.argv[1], sys.argv[2])
