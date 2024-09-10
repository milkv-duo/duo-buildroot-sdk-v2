import numpy as np
import glob
import os
import sys
import tqdm
def convert_file(srcf,dstf):
    data = np.load(srcf)
    if len(data.shape) == 2:
        data = (data[0,:] + data[1,:])*0.5
    print('datashape:',data.shape)
    dst_len = int(16000*3)
    data = data.flatten()
    if len(data) < dst_len:
        return None
    data = data[:dst_len]
    data = data*32767
    data = np.clip(data,-32768,32767)
    data = data.astype(np.int16)
    dstd = os.path.split(dstf)[0]
    os.makedirs(dstd,exist_ok=True)
    data.tofile(dstf)
    return dstf


def append_path(str_dir):
    if str_dir[-1] != '/':
        str_dir = str_dir + '/'
    return str_dir


def convert_files(src_dir,dst_dir):
    src_files = glob.glob(src_dir+'/**/*.npy',recursive=True)
    os.makedirs(dst_dir,exist_ok=True)
    dstf = dst_dir + '/files_list.txt'
    convert_files = []
    pbar = tqdm.tqdm(src_files)
    for srcf in pbar:
        dstf = srcf.replace(src_dir,dst_dir).replace('.npy','.bin')
        convertf = convert_file(srcf,dstf)
        if not convertf is None:
            filename = convertf.replace(dst_dir,'')
            convert_files.append(filename)
    
    with open(dstf,'w') as f:
        for cf in convert_files:
            f.write(cf+'\n')
    

def scan_files_list(src_root):
    src_files = glob.glob(src_root+'/**/*.bin',recursive=True)
    convert_files = []
    pbar = tqdm.tqdm(src_files)
    for srcf in pbar:
        dstf = srcf.replace(src_root,'')
        convert_files.append(dstf)
    
    dstf = src_root + '/files_list.txt'
    with open(dstf,'w') as f:
        for cf in convert_files:
            f.write(cf+'\n')
if __name__ == "__main__":
    scan_files_list(sys.argv[1])
    sys.exit()
    src_dir = append_path(sys.argv[1])
    dst_dir = append_path(sys.argv[2])

    convert_files(src_dir,dst_dir)