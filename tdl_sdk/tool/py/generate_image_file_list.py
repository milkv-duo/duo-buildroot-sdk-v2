import glob
import sys
import cv2
import tqdm
def scan_image_files(img_root,dst_list_file):
    if img_root[-1] != '/':
        img_root += '/'
    
    img_files = glob.glob(img_root+'/**/*.jpg',recursive=True)
    relative_files = [imgf[len(img_root):] for imgf in img_files]
    with open(dst_list_file,'w') as f:


        for rf in relative_files:
            f.write(rf+'\n')

def scan_image_files_size_filter(img_root,dst_list_file):
    if img_root[-1] != '/':
        img_root += '/'
    
    img_files = glob.glob(img_root+'/**/*.jpg',recursive=True)
    relative_files = [imgf[len(img_root):] for imgf in img_files]
    with open(dst_list_file,'w') as f:
        pbar = tqdm.tqdm(range(len(img_files)))
        for i in pbar:
            imgf = img_files[i]
            rf = relative_files[i]
            img = cv2.imread(imgf)
            h,w,_ = img.shape
            strprefix = ''
            if h > 1080 or w > 1920:
                strprefix='#'
            f.write(strprefix+rf+'\n')

def scan_image_files1(img_root,dst_list_file):
    if img_root[-1] != '/':
        img_root += '/'
    sub_dirs = ['1_h264','3_h264','31_h264','39_h264','40_h264','41_h264','seq_001','seq_002']
    img_files = []
    for sd in sub_dirs:
        img_files += glob.glob(img_root+'/'+sd+'/*.jpg')
    img_files = sorted(img_files)
    relative_files = [imgf[len(img_root)+1:] for imgf in img_files]
    with open(dst_list_file,'w') as f:
        for i  in range(0,len(relative_files),5):
            f.write(relative_files[i]+'\n')
if __name__ == "__main__":
    scan_image_files_size_filter(sys.argv[1],sys.argv[2])


    