#/bin/bash

CHIPSET="183x"

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

counter=0
result="PASSED"
run() {
  sub_result="PASSED"
  echo -e "\n@@@@ RUN TEST: $@"
  ./"$@" > /dev/null || sub_result="FAILED"
  counter=$((counter+1))
  echo "@@@@ TEST $counter $1 Done: $sub_result"
  if [[ "$sub_result" = "FAILED" ]]; then
    result="FAILED"
  fi
}

model_dir=${model_dir:-/mnt/data/cvimodel}
dataset_dir=${dataset_dir:-/mnt/data/dataset}
asset_dir=${asset_dir:-/mnt/data/asset}


echo "----------------------"
echo -e "regression setting:"
echo -e "model dir: \t\t${model_dir}"
echo -e "dataset dir: \t\t${dataset_dir}"
echo -e "asset dir: \t\t${asset_dir}"
echo "----------------------"

if [[ "$CHIPSET" = "183x" ]]; then
  run reg_daily_fq ${model_dir} ${dataset_dir}/reg_daily_fq ${asset_dir}/daily_reg_FQ.json
  run reg_daily_lpd ${model_dir} ${dataset_dir}/reg_daily_lpd ${asset_dir}/daily_reg_LPD.json
  run reg_daily_lpr ${model_dir} ${dataset_dir}/reg_daily_lpr ${asset_dir}/daily_reg_LPR.json
  run reg_daily_mot ${model_dir} ${dataset_dir}/reg_daily_mot ${asset_dir}/daily_reg_MOT.json
  run reg_daily_reid ${model_dir} ${dataset_dir}/reg_daily_reid ${asset_dir}/daily_reg_ReID.json
  run reg_daily_td ${model_dir} ${dataset_dir}/reg_daily_td ${asset_dir}/daily_reg_TD.json
  run reg_daily_thermal_fd ${model_dir} ${dataset_dir}/reg_daily_thermal_fd ${asset_dir}/daily_reg_ThermalFD.json
  run reg_daily_mobiledet ${model_dir} ${dataset_dir}/reg_daily_mobildet ${asset_dir}/daily_reg_mobiledet.json

  run reg_daily_fr ${model_dir} ${dataset_dir}/reg_daily_fr ${asset_dir}/daily_reg_FR.json
  run reg_daily_mask_classification ${model_dir} ${dataset_dir}/reg_daily_mask_classification ${asset_dir}/daily_reg_MaskClassification.json

#  run reg_daily_alphapose ${model_dir} ${dataset_dir}/reg_daily_alphapose ${asset_dir}/reg_daily_alphapose.json
#  run reg_daily_fall ${model_dir} ${dataset_dir}/reg_daily_fall ${asset_dir}/reg_daily_fall.json
  run reg_daily_liveness ${model_dir} ${dataset_dir}/reg_daily_liveness ${asset_dir}/reg_daily_liveness.json
  run reg_daily_retinaface ${model_dir} ${dataset_dir}/reg_daily_retinaface ${asset_dir}/reg_daily_retinaface.json

#  run reg_daily_es_classification ${model_dir} ${dataset_dir}/reg_daily_es_classification ${asset_dir}/reg_daily_es_classification.json
  run reg_daily_eye_classification ${model_dir} ${dataset_dir}/reg_daily_eye_classification ${asset_dir}/reg_daily_eye_classification.json
  run reg_daily_fl ${model_dir} ${dataset_dir}/reg_daily_fl/test_face ${asset_dir}/reg_daily_fl.json
  run reg_daily_incarod ${model_dir} ${dataset_dir}/reg_daily_incarod ${asset_dir}/reg_daily_incarod.json
#  run reg_daily_yawn_classification ${model_dir} ${dataset_dir}/reg_daily_yawn_classification ${asset_dir}/reg_daily_yawn_classification.json
elif [[ "$CHIPSET" = "182x" ]]; then
  run reg_daily_mobiledet ${model_dir} ${dataset_dir}/reg_daily_mobildet ${asset_dir}/daily_reg_mobiledet.json
  run reg_daily_fq ${model_dir} ${dataset_dir}/reg_daily_fq ${asset_dir}/daily_reg_FQ.json
  run reg_daily_lpd ${model_dir} ${dataset_dir}/reg_daily_lpd ${asset_dir}/daily_reg_LPD.json
  run reg_daily_lpr ${model_dir} ${dataset_dir}/reg_daily_lpr ${asset_dir}/daily_reg_LPR.json
  run reg_daily_mot ${model_dir} ${dataset_dir}/reg_daily_mot ${asset_dir}/daily_reg_MOT.json
  run reg_daily_reid ${model_dir} ${dataset_dir}/reg_daily_reid ${asset_dir}/daily_reg_ReID.json
  run reg_daily_td ${model_dir} ${dataset_dir}/reg_daily_td ${asset_dir}/daily_reg_TD.json
  run reg_daily_thermal_fd ${model_dir} ${dataset_dir}/reg_daily_thermal_fd ${asset_dir}/daily_reg_ThermalFD.json

  run reg_daily_fr ${model_dir} ${dataset_dir}/reg_daily_fr ${asset_dir}/daily_reg_FR.json
  run reg_daily_mask_classification ${model_dir} ${dataset_dir}/reg_daily_mask_classification ${asset_dir}/daily_reg_MaskClassification.json

#  run reg_daily_alphapose ${model_dir} ${dataset_dir}/reg_daily_alphapose ${asset_dir}/reg_daily_alphapose.json
#  run reg_daily_fall ${model_dir} ${dataset_dir}/reg_daily_fall ${asset_dir}/reg_daily_fall.json
  run reg_daily_liveness ${model_dir} ${dataset_dir}/reg_daily_liveness ${asset_dir}/reg_daily_liveness.json
  run reg_daily_retinaface ${model_dir} ${dataset_dir}/reg_daily_retinaface ${asset_dir}/reg_daily_retinaface.json

#  run reg_daily_es_classification ${model_dir} ${dataset_dir}/reg_daily_es_classification ${asset_dir}/reg_daily_es_classification.json
  run reg_daily_eye_classification ${model_dir} ${dataset_dir}/reg_daily_eye_classification ${asset_dir}/reg_daily_eye_classification.json
  run reg_daily_fl ${model_dir} ${dataset_dir}/reg_daily_fl/test_face ${asset_dir}/reg_daily_fl.json
  run reg_daily_incarod ${model_dir} ${dataset_dir}/reg_daily_incarod ${asset_dir}/reg_daily_incarod.json
#  run reg_daily_yawn_classification ${model_dir} ${dataset_dir}/reg_daily_yawn_classification ${asset_dir}/reg_daily_yawn_classification.json
fi

echo -e "\nDaily Regression Result: ${result}"