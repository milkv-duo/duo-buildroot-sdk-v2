import re
import sys
import time
import os
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

DAILYRELEASE_PATH="/data/dailyrelease"
net_names = [
    "resnet50",
    "resnet18",
    "mobilenet_v1",
    "mobilenet_v2",
    "squeezenet_v1.1",
    "shufflenet_v2",
    "googlenet",
    "inception_v3",
    "inception_v4",
    "vgg16",
    "densenet_121",
    "densenet_201",
    "senet_r50",
    "resnext50",
    "res2net50",
    "ecanet50",
    "efficientnet_b0",
    "efficientnet-lite_b0",
    "nasnet_mobile",
    "retinaface_mnet25",
    "retinaface_mnet25_600",
    "retinaface_res50",
    "ssd300",
    "mobilenet_ssd",
    "yolo_v1_448",
    "yolo_v2_416",
    "yolo_v2_1080p",
    "yolo_v3_320",
    "yolo_v3_416",
    "yolo_v3_tiny",
    "yolo_v3_608",
    "yolo_v3_spp",
    "yolo_v4",
    "yolo_v4_s",
    "yolo_v5_s",
    "yolox_s",
    "faster_rcn",
    "arcface_res50",
    "alphapose",
    "espcn_3x",
    "unet",
    "erfnet"
]

def parse_peformance_log(in_log_file):
    records = {}
    with open(in_log_file, 'r') as f:
        lines = f.readlines()

    cur_name = None
    cur_bs = 1
    for line in lines:
        r = re.match('^test\s+?(.*)', line)
        if r:
            cur_name = r.groups()[0]
            # print(cur_name)
            x = re.match('^(.+?)\s+?batch\s+(\d+)$',cur_name)
            bs = 1
            if x:
                cur_name = x.groups()[0]
                bs = int(x.groups()[1])
            if bs == 1 or bs == 4:
                cur_name = cur_name + "_bs" + str(bs)
                records[cur_name] = [0,0,0,0]
            else:
                assert(0)
        else:
            r = re.match('tdma_exe_ms:\s+([\d\.]*)ms,\s+tiu_exe_ms:\s+([\d\.]*)ms,\s+inference_ms:\s+([\d\.]*)ms', line)
            if r:
                tdma, tiu, inference = r.groups()
                records[cur_name] = [inference, 0]
    return records


def parse_best_csv(best_csv):
    best_records = {}
    # print("parse best performance log file")
    with open(best_csv, 'r') as f:
        lines = f.readlines()
    for line in lines:
        r = re.match('^(.+?_bs\d+),([\d\.]*)', line)
        if r:
            cur_name, inference = r.groups()
            best_records[cur_name] = inference
            # print(best_records)
    return best_records

def compare_record(cur_record, best_record):
    for k, best_perf in best_record.items():
        if k in cur_record:
            cur_perf = cur_record[k]
            cur_perf[1] = (float(best_perf) - float(cur_perf[0]))*100.0/float(best_perf)
            print("Net: {0} Best Cycle: {1}ms, Current Cycle: {2}ms Improve:{3:.2f}%".format(k,
                   best_perf, cur_perf[0], cur_perf[1]))
        else:
            # current performance fail on some network
            print("[Warning] lost today's performance data for network: ", k)

def output_csv(cur_records, best_records, out_file):
    with open(out_file, 'w') as f:
        f.write('{}\n'.format(time.strftime("%Y-%m-%d",time.localtime(time.time()))))
        f.write('Net,Best Cycle(ms),Current Cycle(ms),Improve(%)\n')
        for name, best_record in best_records.items():
            if name not in cur_records:
                f.write("{0},{1},{2},{3:.2f}%, NNNNNN\n".format(name, best_record, -100.0, -100.0))
                continue
            cur_record = cur_records[name]
            f.write("{0},{1},{2},{3:.2f}%".format(name, best_record, cur_record[0], cur_record[1]))
            if cur_record[1] > 1.0:
                print("[Congs]Net: {0} Best Cycle: {1}ms, Current Cycle: {2}ms, Improve: {3:.2f}%".format(
                        name, best_records[name], cur_record[0], cur_record[1]))
                f.write(",YYYYYY\n")
            elif cur_record[1] < -1.0:
                print("[Warning]Net: {0} Best Cycle: {1}ms, Current Cycle: {2}ms, Improve: {3:.2f}%".format(
                        name, best_records[name], cur_record[0], cur_record[1]))
                f.write(",NNNNNN\n")
            else:
                f.write("\n")

    # generate data result according to net_names
    file_for_release = os.path.splitext(out_file)[0] + "_release.csv"
    with open(file_for_release, 'w') as f:
        f.write('{}\n'.format(time.strftime("%Y-%m-%d",time.localtime(time.time()))))
        f.write('Net,Best Cycle(ms),Current Cycle(ms),Improve(%)\n')
        for net in net_names:
            best_data = "--"
            cur_data = "--"
            cur_improve = "--"
            b_best = False
            b_cur = False
            net = net + "_bs1"
            if net in best_records:
                best_data = best_records[net]
                b_best = True
            if net in cur_records:
                cur_data = cur_records[net][0]
                cur_improve = str(cur_records[net][1])
                b_cur = True

            if b_cur == True:
                f.write("{0},{1},{2},{3:.2f}%".format(net, best_data, cur_data, float(cur_improve)))
            else:
                f.write("{0},{1},{2},{3}".format(net, best_data, cur_data, cur_improve))

            if b_best == True and b_cur == True:
                if cur_records[net][1] > 1.0:
                    f.write(",YYYYYY")
                elif cur_records[net][1] < -1.0:
                    f.write(",NNNNNN")
            f.write("\n")

def get_folder_list(chip_type):
    file_path = DAILYRELEASE_PATH
    dir_list = os.listdir(file_path)
    valid_folder = []
    if not dir_list:
        print("get data release file failed.")
        exit
    else:
        dir_list = sorted(dir_list, key=lambda x: os.path.getmtime(os.path.join(file_path, x)))
        for d in dir_list:
            perf_path = os.path.join(os.path.join(file_path, d), "perf_result_"+chip_type)
            perf_file = os.path.join(perf_path, "performance.log")
            print("perf_file: ", perf_file)
            if os.path.exists(perf_file):
                valid_folder.append(d)

    if (len(valid_folder) > 30):
        valid_folder = valid_folder[-30:]
    return valid_folder

def get_perf_data(folder_list, chip_type, best_records):
    all_perf_data = {}
    all_date = []
    for p_fold in folder_list:
        perf_path = os.path.join(os.path.join(DAILYRELEASE_PATH, p_fold), "perf_result_"+chip_type)
        p_file = os.path.join(perf_path, "performance.log")
        cur_records = parse_peformance_log(p_file)

        for net, data in best_records.items():
            if net in cur_records:
                if net in all_perf_data:
                    all_perf_data[net].append(float(cur_records[net][0]))
                else:
                    all_perf_data[net] = [float(cur_records[net][0])]
            else:
                if net in all_perf_data:
                    all_perf_data[net].append(float(data))
                else:
                    all_perf_data[net] = [float(data)]

        all_date.append(p_fold)
    return all_date, all_perf_data

def draw_perf_history_graph(daily_date, daily_perf_data):
    plt.rcParams['savefig.dpi'] = 500
    plt.rcParams['figure.dpi'] = 300
    import itertools
    marker = itertools.cycle(("8","s","P","*", "D", "|", 4,5,6,7,8,9,10))

    # sub graph 0: 0~5ms
    plt.subplot(2,2,1)
    plt.xticks(rotation=45, fontsize=2)
    plt.yticks(fontsize=2)
    for net_name, net_daily_perf in daily_perf_data.items():
        if net_daily_perf[0] < 5.0:
            plt.plot(daily_date, net_daily_perf, marker=next(marker),
                linewidth=0.5, linestyle='--', label=net_name, markersize=2)
            plt.legend(fontsize=2, bbox_to_anchor=(0, 1.02), loc='lower left', ncol=4)
    # sub graph 1: 5~20ms
    plt.subplot(2,2,2)
    plt.xticks(rotation=45, fontsize=2)
    plt.yticks(fontsize=2)
    for net_name, net_daily_perf in daily_perf_data.items():
        if net_daily_perf[0] > 5.0 and net_daily_perf[0] < 20.0:
            plt.plot(daily_date, net_daily_perf, marker=next(marker),
                linewidth=0.5, linestyle='--', label=net_name, markersize=2)
            plt.legend(fontsize=2, bbox_to_anchor=(0, 1.02), loc='lower left', ncol=4)
    # sub graph 2: 20~100ms
    plt.subplot(2,2,3)
    plt.xticks(rotation=45, fontsize=2)
    plt.yticks(fontsize=2)
    for net_name, net_daily_perf in daily_perf_data.items():
        if net_daily_perf[0] > 20.0 and net_daily_perf[0] < 100.0:
            plt.plot(daily_date, net_daily_perf, marker=next(marker),
                linewidth=0.5, linestyle='--', label=net_name, markersize=2)
            plt.legend(fontsize=2, bbox_to_anchor=(0, 1.02), loc='lower left', ncol=4)
    # sub graph 4: 100ms~
    plt.subplot(2,2,4)
    plt.xticks(rotation=45, fontsize=2)
    plt.yticks(fontsize=2)
    for net_name, net_daily_perf in daily_perf_data.items():
        if net_daily_perf[0] > 100:
            plt.plot(daily_date, net_daily_perf, marker=next(marker),
                linewidth=0.5, linestyle='--', label=net_name, markersize=2)
            plt.legend(fontsize=2, bbox_to_anchor=(0, 1.02), loc='lower left', ncol=4)
    plt.show()
    plt.savefig('./performance_history.jpg')

    with open('performance_history.csv', 'w') as f:
        f.write('Inference Cycle(ms),{}\n'.format(time.strftime("%Y-%m-%d",time.localtime(time.time()))))
        f.write('Net')
        for d in daily_date:
            f.write(",{}".format(d))
        f.write("\n")
        for net_name, net_daily_perf in daily_perf_data.items():
            f.write("{}".format(net_name))
            for net_perf in net_daily_perf:
                f.write(",{}".format(net_perf))
            f.write("\n")

# genereate the perf graph of the past 30 days
def generate_perf_graph(chip_type, best_records):
    # get file list of past 30 days
    folder_list = get_folder_list(chip_type)
    # get performance data of past 30 days
    daily_date, daily_perf_data = get_perf_data(folder_list, chip_type, best_records)
    # print(daily_date)
    # print(daily_perf_data)
    # dump performance graph
    draw_perf_history_graph(daily_date, daily_perf_data)

if __name__ == '__main__':
    if len(sys.argv) != 5:
        print("USAGE: sys.argv[0] best_performance.csv src_log_file out_result.csv cv181x/cv182x/cv183x")
        exit(1)
    best_records = parse_best_csv(sys.argv[1])
    cur_records = parse_peformance_log(sys.argv[2])
    compare_record(cur_records, best_records)
    output_csv(cur_records, best_records, sys.argv[3])
    generate_perf_graph(sys.argv[4], best_records)