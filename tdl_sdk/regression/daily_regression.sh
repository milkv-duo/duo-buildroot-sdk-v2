#!/bin/sh

CHIPSET="${CHIP:=181x}"
CHIPSET=$(echo ${CHIP} | tr '[:upper:]' '[:lower:]')

print_usage() {
  echo ""
  echo "Usage: daily_regression.sh [-m] [-d] [-a] [-h]"
  echo ""
  echo "Options:"
  echo -e "\t-m, cvimodel directory (default: /mnt/data/cvimodel)"
  echo -e "\t-d, dataset directory (default: /mnt/data/dataset)"
  echo -e "\t-a, json data directory (default: /mnt/data/asset)"
  echo -e "\t-h, help"
}

while getopts "m:d:a:h?" opt; do
  case ${opt} in
    m)
      model_dir=$OPTARG
      ;;
    d)
      dataset_dir=$OPTARG
      ;;
    a)
      asset_dir=$OPTARG
      ;;
    h)
      print_usage
      exit 0
      ;;
    \?)
      print_usage
      exit 128
      ;;
  esac
done

model_dir=${model_dir:-/mnt/data/cvimodel}
dataset_dir=${dataset_dir:-/mnt/data/dataset}
asset_dir=${asset_dir:-/mnt/data/asset}

if [ -f "/sys/kernel/debug/ion/cvi_carveout_heap_dump/total_mem" ]; then
  total_ion_size=$(cat /sys/kernel/debug/ion/cvi_carveout_heap_dump/total_mem)
elif [ -f "/sys/kernel/debug/ion/cvi_npu_heap_dump/total_mem" ]; then
  total_ion_size=$(cat /sys/kernel/debug/ion/cvi_npu_heap_dump/total_mem)
  CHIP_ARCH="CV186X"
else
  # if ion size is unknown then execute basic tests.
  total_ion_size=20000000
fi

test_suites=""

# ION requirement >= 20 MB
if [ "$total_ion_size" -gt "20000000" ]; then
  test_suites="CoreTestSuite.*"
  test_suites="${test_suites}:FaceRecognitionTestSuite.*"
  test_suites="${test_suites}:MaskClassification.*"
  test_suites="${test_suites}:FaceQualityTestSuite.*"
  test_suites="${test_suites}:MultiObjectTrackingTestSuite.*"
  test_suites="${test_suites}:ReIdentificationTestSuite.*"
  test_suites="${test_suites}:LivenessTestSuite.*"
  test_suites="${test_suites}:RetinafaceTestSuite.*"
  test_suites="${test_suites}:FaceMaskDetectionTestSuite.*"
  test_suites="${test_suites}:RetinafaceIRTestSuite.*"
  test_suites="${test_suites}:RetinafaceHardhatTestSuite.*"
  test_suites="${test_suites}:IncarTestSuite.*"
  test_suites="${test_suites}:ESCTestSuite.*"
  test_suites="${test_suites}:EyeCTestSuite.*"
  test_suites="${test_suites}:YawnCTestSuite.*"
  test_suites="${test_suites}:SoundCTestSuite.*"
  test_suites="${test_suites}:FLTestSuite.*"
  #test_suites="${test_suites}:TamperDetectionTestSuite.*"
fi

# ION requirement >= 35 MB
if [ "$total_ion_size" -gt "35000000" ]; then
test_suites="${test_suites}:LicensePlateDetectionTestSuite.*"
test_suites="${test_suites}:LicensePlateRecognitionTestSuite.*"
test_suites="${test_suites}:ThermalFaceDetectionTestSuite.*"
test_suites="${test_suites}:ThermalPersonDetectionTestSuite.*"
test_suites="${test_suites}:FeatureMatchingTestSuite.*"
test_suites="${test_suites}:MotionDetection*"
fi

# ION requirement >= 60 MB
if [ "$total_ion_size" -gt "60000000" ]; then
test_suites="${test_suites}:MobileDetTestSuite.*"
fi

# ION requirement >= 70 MB
if [ "$total_ion_size" -gt "70000000" ]; then
test_suites="${test_suites}:FallTestSuite.*"
test_suites="${test_suites}:PersonPet_DetectionTestSuite.*"
test_suites="${test_suites}:Hand_DetectionTestSuite.*"
test_suites="${test_suites}:Meeting_DetectionTestSuite.*"
test_suites="${test_suites}:People_Vehicle_DetectionTestSuite.*"
test_suites="${test_suites}:FaceRecognitionTestSuite.*"
test_suites="${test_suites}:LicensePlateDetectionTestSuite.*"
test_suites="${test_suites}:ScrfdDetTestSuite.*"
test_suites="${test_suites}:Hand_ClassificationTestSuite.*"
test_suites="${test_suites}:HardhatDetTestSuite.*"
test_suites="${test_suites}:MobileDetV2TestSuite.*"
fi

if [ $CHIP_ARCH == "CV186X" ];then
  test_suites="${test_suites}:PersonPet_DetectionTestSuite.*"
  test_suites="${test_suites}:Hand_DetectionTestSuite.*"
  test_suites="${test_suites}:Meeting_DetectionTestSuite.*"
  test_suites="${test_suites}:People_Vehicle_DetectionTestSuite.*"
  test_suites="${test_suites}:FaceRecognitionTestSuite.*"
  test_suites="${test_suites}:LicensePlateDetectionV2TestSuite.*"
  test_suites="${test_suites}:ScrfdDetTestSuite.*"
  test_suites="${test_suites}:Hand_ClassificationTestSuite.*"
  test_suites="${test_suites}:HardhatDetTestSuite.*"
  test_suites="${test_suites}:MobileDetectionV2TestSuite.*"
fi
echo "----------------------"
echo -e "regression setting:"
echo -e "model dir: \t\t${model_dir}"
echo -e "dataset dir: \t\t${dataset_dir}"
echo -e "asset dir: \t\t${asset_dir}"
echo -e "CHIPSET: \t\t${CHIPSET}"
echo -e "ION size: \t\t${total_ion_size} bytes"

echo "Test Suites to be executed:"
test_suites_separated=${test_suites//":"/ }
for suite_name in ${test_suites_separated}
do
    echo -e "\t${suite_name}"
done
echo "----------------------"

./test_main ${model_dir} ${dataset_dir} ${asset_dir} --gtest_filter=$test_suites