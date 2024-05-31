#!/usr/bin/python3
import argparse
import sys
import paramiko
import re
from ssh_tool import SshTool

class SampleTestCase:
    def __init__(self, ssh, models_path, sdk_path):
        self.ssh = ssh
        self.command = '''
            export MODEL_PATH={}
            cd {}
            . ./envs_tpu_sdk.sh
            cd samples/
        '''.format(models_path, sdk_path)

    def __parse_result(self, outputs):
        i = 0
        results = []
        while i < len(outputs):
            if outputs[i].startswith('------'):
                j = i + 1
                while j < len(outputs) and \
                      not outputs[j].startswith('------'):
                    results.append(outputs[j])
                    j += 1
                i = j + 1
            else:
                i += 1
        if len(results) == 0:
            raise Exception("no results")
        return results

    def run(self, shell_script, validate_fn, **kargs):
        print("\n#################", shell_script)
        command = self.command + './{}'.format(shell_script)
        err, outputs = self.ssh.exec(command)
        if err == 0:
            results = self.__parse_result(outputs)
            print(results)
            return validate_fn(results, **kargs)
        print("ERROR, shell excute failed. ret: {}".format(err))
        return 1


def validate_alphapose_result(outputs, poses_wanted):
    line = outputs[0]
    poses_detected = int(re.match('^\s*(\d+)', line).groups()[0])
    if poses_detected != poses_wanted:
        print("FAILED, {} poses should be detected, but only has {}".format(
                poses_wanted, poses_detected))
        return 1
    else:
        print('PASSED')
        return 0

def validate_classifier_result(outputs, id, score):
    for line in outputs:
        _score, _id = re.match('^\s*([0-9.]+), idx (\d+)', line).groups()
        if int(_id) == id and float(_score) > score:
            print('PASSED')
            return 0
    print('FAILED, id={}, score > {}'.format(id, score))
    return 1

def validate_detector_result(outputs, objects_wanted):
    line = outputs[0]
    objects_detected = int(re.match('^(\d)', line).groups()[0])
    if objects_detected != objects_wanted:
        print("FAILED, {} objects should be detected, but only has {}".format(
              objects_wanted, objects_detected))
        return 1
    else:
        print('PASSED')
        return 0

def validate_insightface_result(outputs, similarity):
    for target, line in zip(similarity, outputs):
        gotten = re.match('^Similarity: ([0-9.\-]+)', line).groups()[0]
        if target > 0.1 and float(gotten) < target:
            print('FAILED, similarity {} < target:{}'.format(gotten, target))
            return 1
        elif target <= 0.1 and float(gotten) > target:
            print('FAILED, similarity {} > target:{}'.format(gotten, target))
            return 1
    print('PASSED')
    return 0

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="samples_test")
    parser.add_argument("-t", "--target", type=str, required=True)
    parser.add_argument("-m", "--models_path", type=str)
    parser.add_argument("-s", "--sdk_path", type=str)
    args = parser.parse_args()

    ssh = SshTool(args.target)
    testcase = SampleTestCase(ssh, args.models_path, args.sdk_path)
    cnt = 0
    cnt += testcase.run('run_alphapose.sh', validate_alphapose_result, poses_wanted=5)
    cnt += testcase.run('run_alphapose_fused_preprocess.sh', validate_alphapose_result, poses_wanted=5)
    cnt += testcase.run('run_classifier.sh', validate_classifier_result, id=285, score=0.36)
    cnt += testcase.run('run_classifier_fused_preprocess.sh', validate_classifier_result, id=285, score=0.32)
    cnt += testcase.run('run_classifier_yuv420.sh', validate_classifier_result, id=285, score=0.34)
    cnt += testcase.run('run_detector.sh', validate_detector_result, objects_wanted=3)
    cnt += testcase.run('run_detector_fused_preprocess.sh', validate_detector_result, objects_wanted=3)
    cnt += testcase.run('run_insightface.sh', validate_insightface_result, similarity=[0.75, 0.8, 0.81, -0.01, -0.08, 0.03])
    cnt += testcase.run('run_insightface_fused_preprocess.sh', validate_insightface_result, similarity=[0.75, 0.8, 0.76, 0, -0.08, 0.04])
    if cnt > 0:
        print('{} testcases failed'.format(cnt))
    sys.exit(cnt)