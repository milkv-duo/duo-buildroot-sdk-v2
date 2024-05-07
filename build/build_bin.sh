#!/bin/bash

KEYSERVER="10.80.65.18"
KEYSERVER_SSHKEY_PATH="$ATF_PATH/tools/build_script/service_sign@cvi_keyserver.pem"

function sign_fip_atf_on_keyserver()
{(
  # Sign fip with ROM key1 from keyserver
  chmod 400 "$KEYSERVER_SSHKEY_PATH"
  msg=$(scp -i "$KEYSERVER_SSHKEY_PATH" "$OUTPUT_DIR/fip.bin" "service_sign@${KEYSERVER}:fip.bin" 2>&1)
  cat <<<$msg
  token=$(command grep -E 'TOKEN:.{32}' -o <<<$msg); token=${token#TOKEN:}
  echo "TOKEN: $token"
  if [ -z "$token" ]; then
    return 1
  fi

  ssh -i "$KEYSERVER_SSHKEY_PATH" "service_sign@${KEYSERVER}" sign_fip "--chip=cv1835 --token=${token}"
  scp -i "$KEYSERVER_SSHKEY_PATH" "service_sign@${KEYSERVER}:fip_ID${token}_signed_encrypted.bin" "$OUTPUT_DIR/fip_signed_encrypted.bin"
)}

function gen_fip_bin_with_cid()
{(
  local tbbr="$2"

  printf "\e[1;34;47m Run %s() function \e[0m\n" "${FUNCNAME[0]}"

  source build/envsetup_soc.sh f
  defconfig "${CHIP}_${BOARD}"
  setconfig ATF_SRC=y

  source build/build_bin.sh

  local ATF_FIP_PATH="${ATF_PATH}/build/${CHIP}_${SUBTYPE}/release/fip.bin"

  clean_bld
  clean_atf
  build_atf
	command cp "${ATF_FIP_PATH}" "${OUTPUT_DIR}/fip.bin"

  {
    printf "bm_bld:\n"
    git -C "${BM_BLD_PATH}" log --pretty=oneline -n 1
    printf "arm-trusted-firmware:\n"
    git -C "${ATF_PATH}" log --pretty=oneline -n 1
  } > "$TOP_DIR/git_version.txt"

  atf_ver=$(git -C "${ATF_PATH}" rev-parse --short HEAD)
  bld_ver=$(git -C "${BM_BLD_PATH}" rev-parse --short HEAD)
  git_ver="${atf_ver}_${bld_ver}"

  command cp -f $OUTPUT_DIR/fip.bin $OUTPUT_DIR/fip.bin.$git_ver

  if [ "$tbbr" != "notbbr" ]; then
    sign_fip_atf_on_keyserver
    command cp "$OUTPUT_DIR/fip_signed_encrypted.bin" "$OUTPUT_DIR/fip_key1.bin.$git_ver"
  fi

  FIP_BIN_FOLDER_NAME=${CHIP}_${SUBTYPE}_${BOARD}

  # copy fip.bin to NAS server
  echo "Copy fip.bin to network folder: /home/git_bin/fip/${FIP_BIN_FOLDER_NAME}/fip.bin"
  command mkdir -p /home/git_bin/fip/${FIP_BIN_FOLDER_NAME}
  command cp -f $OUTPUT_DIR/fip.bin.$git_ver /home/git_bin/fip/${FIP_BIN_FOLDER_NAME}/fip.bin.$git_ver
  command cp -f $OUTPUT_DIR/fip.bin.$git_ver /home/git_bin/fip/${FIP_BIN_FOLDER_NAME}/fip.bin
  if [ "$tbbr" != "notbbr" ]; then
    command cp -f "$OUTPUT_DIR/fip_key1.bin.$git_ver" /home/git_bin/fip/${FIP_BIN_FOLDER_NAME}/fip_key1.bin.$git_ver
    command cp -f "$OUTPUT_DIR/fip_key1.bin.$git_ver" /home/git_bin/fip/${FIP_BIN_FOLDER_NAME}/fip_key1.bin
  fi
  command cp -f $TOP_DIR/git_version.txt /home/git_bin/fip/${FIP_BIN_FOLDER_NAME}/git_version.txt
)}

function gen_fip_bin_with_cid_all_boards()
{(
  printf "\e[1;34;47m Run %s() function \e[0m\n" "${FUNCNAME[0]}"

  chip_list=("${chip_cv183x[@]}" "${chip_cv75x1[@]}" "${chip_cv952x[@]}")

  for c in "${!chip_list[@]}"; do
    export CHIP=${chip_list[$c]}
    local -a 'board_sel=("${'"${CHIP}"'_board_sel[@]}")'

    for b in "${!board_sel[@]}"; do
      export BOARD=${board_sel[$b]}
      if [[ "$BOARD" =~ .*rtos.* ]]; then
        continue
      fi
      (
        cd $TOP_DIR || exit
        gen_fip_bin_with_cid $BOARD
      )
    done
  done
)}

function gen_fip_bin_wo_tbbr_with_cid_all_boards()
{(
  chip_list=("${chip_cv182x[@]}")

  for c in "${!chip_list[@]}"; do
    export ATF_SRC=1
    export CHIP=${chip_list[$c]}
    export SUBTYPE=asic
    export ATF_CRC=1
    export IMG_ENC=0
    export ATF_TBBR=0

    local -a 'board_sel=("${'"${CHIP}"'_board_sel[@]}")'

    for b in "${!board_sel[@]}"; do
      export BOARD=${board_sel[$b]}
      (
        cd $TOP_DIR || exit
        source build/envsetup_soc.sh f
        source build/build_bin.sh
        gen_fip_bin_with_cid $BOARD notbbr
      )
    done
  done
)}
