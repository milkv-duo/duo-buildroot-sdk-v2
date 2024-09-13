import json
import os
import sys
from tqdm import tqdm
import cv2

def draw_res(img_root, res_dir, draw_dir):
    file_list = os.listdir(img_root)
    os.makedirs(draw_dir, exist_ok=True)
    for img_name in tqdm(file_list):
        txt_path = os.path.join(res_dir, os.path.splitext(img_name)[0] + ".txt")
        if not os.path.exists(txt_path):
            continue
        img_path = os.path.join(img_root, img_name)
        img = cv2.imread(img_path)
        with open(txt_path, "r") as f:
            txt = f.read().split("\n")
        for line in txt:
            data = line.split(" ")
            while "" in data:
                data.remove("")
            if not data:
                continue
            category, score, x1, y1, x2, y2 = data
            if float(score) < 0.5:
                continue
            box = [int(float(x)) for x in [x1, y1, x2, y2]]
            img = cv2.rectangle(img, (box[0], box[1]), (box[2], box[3]), color=(0, 0, 255), thickness=2)
            cv2.putText(img, str(score), (box[0], box[1]), color=(0, 0, 255), fontFace=cv2.FONT_HERSHEY_SIMPLEX, fontScale=1, thickness=1)
        cv2.imwrite(os.path.join(draw_dir, img_name), img)

## the script will draw person dectect model output rectangle
## argv[1]: dectect image dir, argv[2] :output resource txt, argv[2] :output dir
## eg: python cvi_draw_pd.py ./image ./res ./output
if __name__ == "__main__":
    draw_res(sys.argv[1], sys.argv[2], sys.argv[3])

