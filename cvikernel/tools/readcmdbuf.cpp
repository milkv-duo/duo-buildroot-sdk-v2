#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fstream>
#include <glog/logging.h>
#ifdef BM1880v2
#include <map>
#include <iostream>
#include <string>
#include "../src/bm1880v2/kernel_1880v2.h"
//#include "../../include/builder/Builder.hpp"
#include <fcntl.h>
#include <fstream>  // NOLINT(readability/streams)
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
using namespace std;

using google::protobuf::io::FileInputStream;

#else /* ! if defined(CHIP) && CHIP == BM1880v2*/

#include <bmkernel/bm_kernel_legacy.h>
#endif /* if defined(CHIP) && CHIP == BM1880v2*/

int get_file_length(std::fstream& f)
{
  f.seekg (0, f.end);
  int length = f.tellg();
  f.seekg (0, f.beg);
  return length;
}

#if defined(CHIP) && CHIP == BM1880v2
static int bm1880v2_get_engine_desc_length(uint32_t engine_id)
{
  switch (engine_id) {
    case BMK1880v2_TIU:
      return TIU_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    case BMK1880v2_TDMA:
      return TDMA_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    case BMK1880v2_CPU:
      return CPU_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    default:
      ASSERT(0);
  }
}
static void parseCmdBuf(char* cmdbuf_path, map<int, string> & map_layer_id_name,
    char* output_file_path) {

  std::fstream f_cmdbuf(cmdbuf_path, std::ios::in | std::ios::binary);

  size_t i = 0;
  size_t cmdbuf_size = get_file_length(f_cmdbuf);
  //printf("cmdbuf size %zu\n", cmdbuf_size);
  uint8_t *cmdbuf = new uint8_t[cmdbuf_size];
  uint32_t *buf = (uint32_t *)cmdbuf;

  std::ofstream output_file_path_fp;
  output_file_path_fp.open(output_file_path);
  f_cmdbuf.read((char *)cmdbuf, cmdbuf_size);

  while(i < cmdbuf_size / 4) {
    cmd_hdr_t *hdr;
    hdr = (cmd_hdr_t *)(&buf[i]);
    i += sizeof(cmd_hdr_t) / 4;
    const uint32_t *cmd = (uint32_t*)&buf[i];
    ASSERT(hdr->magic == BM_CMB_HDR_MAGIC);
    uint32_t eng_id = hdr->engine_id;
    //uint32_t cmd_len = cmd_hdr_len(hdr) / 4;
    uint32_t cmd_len = bm1880v2_get_engine_desc_length(eng_id) / 4;
    i += cmd_len;

    switch (eng_id) {
      case BMK1880v2_TIU:
        {
          tiu_reg_t tiuTempReg;
          int layer_id;
          string mappined = "";
          map<int, string>::iterator iter;

          parse_tiu_reg(&tiuTempReg, cmd);
          layer_id = tiuTempReg.rsvd3;

          if (output_file_path) {
            iter = map_layer_id_name.find(layer_id);

            if (iter != map_layer_id_name.end()) {
              mappined = iter->second;
              output_file_path_fp << tiuTempReg.cmd_id_tpu << "," << mappined << "\n";
            }
          }
          LOG(INFO) << "CMD [BD  ], <" <<
            tiuTempReg.cmd_id_tpu
            << ">, len " << cmd_len;
        }
        break;
    case BMK1880v2_TDMA:
        {
          tdma_reg_t tdma_reg;
          parse_tdma_reg(&tdma_reg, cmd);
          LOG(INFO) << "[GDMA ], <" << tdma_reg.cmd_id
            << ">, len " << cmd_len;
        }
        break;
    case BMK1880v2_CPU:
        LOG(INFO) << "[CPU], ";
        break;
    default:
        CHECK(0) << "impossible eng_id " << eng_id << " not found";
    }
  }

  if (output_file_path) {
    output_file_path_fp.close();
  }
}

static void read_bm1880v2_cmd(char* cmdbuf_path, char* layer_id_name_mapping_file_path,
  char* output_file_path) {
  string line;
  map<int, string> map_layer_id_name;
  map<int, string>::iterator iter;

  if (layer_id_name_mapping_file_path) {
    std::ifstream file(layer_id_name_mapping_file_path);

    if (file.fail()) {
      CHECK(0) << "File not found: " << cmdbuf_path;
    }

    while (getline(file, line)) {
      istringstream templine(line);
      string data;
      int group_id;
      while (getline(templine, data,',')) {
        group_id = atof(data.c_str());
        break;
      }
      map_layer_id_name[group_id] = line;
    }

  //for(iter = map_layer_id_name.begin(); iter != map_layer_id_name.end(); iter++) {
  //  cout<<iter->first<<" "<<iter->second<<endl;
  //}
  }

  parseCmdBuf(cmdbuf_path, map_layer_id_name, output_file_path);

}
#else /* ! if defined(CHIP) && CHIP == BM1880v2*/

#define DETAILED_CMD
#undef DETAILED_CMD
static inline int desc_get_bd_dcr_type(void *desc)
{
  uint32_t *r = (uint32_t *)desc;
  return (r[BD_CMD_REGI1] >> 8) & 0xff;
}

static const char* desc_get_md_scale_op(void *desc)
{
  uint32_t *r = (uint32_t *)desc;
  int op = (r[BD_CMD_REGI2] >> 19) & 0x3;
  switch(op) {
    case 0: return "ADD";
    case 1: return "SUB";
    case 2: return "MUL";
    case 3: return "DIV";
    default: return "UNKOWN";
  }
}

static inline void desc_get_cmd_id(
    void *desc, int engine_id, uint32_t cmd_len,
    uint32_t& bd_cmd_id, uint32_t& gdma_cmd_id)
{
  uint32_t *r = (uint32_t *)desc;
  if(ENGINE_BD == engine_id) {
    if (cmd_len == 28) { // bd len = 28 in bm1880
      uint64_t data = ((uint64_t*)desc)[0];
      bd_cmd_id = ( data >> 3) & 0x0ffff;
      gdma_cmd_id = (data >> 19) & 0x0ffff;
    } else {
      bd_cmd_id = (r[BD_CMD_REGI1] >> 16) & 0x0ffff;
      gdma_cmd_id = (r[BD_CMD_REGI23] >> 16) & 0x0ffff;
    }
  } else if(ENGINE_GDMA == engine_id) {
    bd_cmd_id = (r[GDMA_CMD_ACCPI1] >> 16) & 0x0ffff;
    gdma_cmd_id = (r[GDMA_CMD_ACCPI0] >> 16) & 0x0ffff;
  }
}

static void parse_gdma_operand_addr(void *desc, uint64_t& src, uint64_t& dst) {
  uint32_t *r = (uint32_t *)desc;
  src = 0;
  dst = 0;
  int direction = (int)((r[GDMA_CMD_ACCPI0] >> GDMA_ACCPI0_DIRECTION_BIT) & 0x3);
  switch (direction){
    case GDMA_DIR_S2L:
      src = r[GDMA_CMD_ACCPI12] + (((uint64_t)(r[GDMA_CMD_ACCPI13] & 0xff000000)) << 8);
      dst = r[GDMA_CMD_ACCPI11];
      break;
    case GDMA_DIR_L2S:
      src = r[GDMA_CMD_ACCPI12];
      dst = r[GDMA_CMD_ACCPI11] + (((uint64_t)(r[GDMA_CMD_ACCPI13] & 0x00ff0000)) << 16);
      break;
    case GDMA_DIR_S2S:
      src = r[GDMA_CMD_ACCPI12] + (((uint64_t)(r[GDMA_CMD_ACCPI13] & 0xff000000)) << 8);
      dst = r[GDMA_CMD_ACCPI11] + (((uint64_t)(r[GDMA_CMD_ACCPI13] & 0x00ff0000)) << 16);
      break;
    case GDMA_DIR_L2L:
      src = r[GDMA_CMD_ACCPI12];
      dst = r[GDMA_CMD_ACCPI11];
      break;
    default:
      break;
  }
}

static std::string bd_type(void *desc)
{
  int dcr_type = desc_get_bd_dcr_type(desc);
  CHECK_LT(dcr_type, NR_DCR_TYPES);

  switch (dcr_type) {
    case DCR_TYPE_CONV:
      return std::string("CONV");
    case DCR_TYPE_MD_SUM:
      return std::string("MD_SUM");
    case DCR_TYPE_MD_LINEAR:
      return std::string("MD_LINEAR");
    case DCR_TYPE_MD_SCALAR:
      return std::string("MD_SCALAR ") + std::string(desc_get_md_scale_op(desc));
    case DCR_TYPE_MD_SFU:
      return std::string("MD_SFU");
    case DCR_TYPE_MD_CMP:
      return std::string("MD_CMP");
    case DCR_TYPE_POOLING_FWD:
      return std::string("MD_POOLING_FWD");
    case DCR_TYPE_POOLING_BWD:
      return std::string("MD_POOLING_BWD");
    case DCR_TYPE_MATRIX_MULTIPLY:
      return std::string("MATMUL");
    case DCR_TYPE_IMAGE_SUM:
      return std::string("IMG_SUM");
    case DCR_TYPE_CONV_COEFF:
      return std::string("CONV_COEFF");
    case DCR_TYPE_CONV_COEFF_MAC:
      return std::string("CONV_COEFF_MAC");
    case DCR_TYPE_LMEM_ARRANGE:
      return std::string("LMEM_ARRANGE");
    case DCR_TYPE_TENSOR_ARITHMETIC:
      return std::string("LMEM_ARITHMETIC");

    default:
      LOG(FATAL) << "Unknown BD dcr_type " << dcr_type;
  }
}

static void parse_cmdbuf(uint8_t *cmdbuf, size_t cmdbuf_size)
{
  uint32_t *buf = (uint32_t *)cmdbuf;
  size_t i = 0;

  while(i < cmdbuf_size / 4) {
    cmd_hdr_t *hdr;
    void *cmd;
    hdr = (cmd_hdr_t *)(&buf[i]);
#ifdef DETAILED_CMD
     printf("magic:%d len: %d engine_id: %d node_id: %d"
            "flags: %d mask: %d cmd[0]:%d\n", (int)hdr->magic, (int)cmd_hdr_len(hdr),
            (int)hdr->engine_id, (int)hdr->__deprecated, (int)hdr->flags, (int)hdr->mask,
            (int)hdr->cmd[0]);
#endif
    i += sizeof(cmd_hdr_t) / 4;
    cmd = (void *)&buf[i];
    uint32_t cmd_len = cmd_hdr_len(hdr) / 4;
    uint64_t src, dst;
    uint32_t bd_cmd_id = 0, gdma_cmd_id = 0;
    desc_get_cmd_id(cmd, hdr->engine_id, cmd_len, bd_cmd_id, gdma_cmd_id);

#ifdef DETAILED_CMD
     uint32_t * c = (uint32_t *)cmd;
#endif

    switch (hdr->engine_id) {
    case ENGINE_BD:
      LOG(INFO) << "CMD [BD  ], <"
                << bd_cmd_id << "," << gdma_cmd_id
                << ">, len " << cmd_len << ", "
                << bd_type(cmd);
#ifdef DETAILED_CMD
     for(size_t k = 0; k < cmd_len; k++)
        LOG(INFO) << "[BD " << k << "]:" << std::hex << c[k];
#endif
      break;
    case ENGINE_GDMA:
      parse_gdma_operand_addr(cmd, src, dst);
      LOG(INFO) << "CMD [GDMA], <"
                << bd_cmd_id << "," << gdma_cmd_id
                << ">, 0x" << std::hex << src
                << " => 0x" << std::hex << dst
                << ", len " << cmd_len;
#ifdef DETAILED_CMD
      for(size_t k = 0; k < cmd_len; k++)
        LOG(INFO) << "[GDMA " << k << "]:" << std::hex << c[k];
#endif
      break;
    case ENGINE_CPU:
      LOG(INFO) << "CMD [ARM ], len " << cmd_len;
#ifdef DETAILED_CMD
      for(size_t k = 0; k < cmd_len; k++)
        LOG(INFO) << "[CPU " << k << "]:" << std::hex << c[k];
#endif
      break;
    default:
      LOG(FATAL) << "UNKNOWN CMD, len " << cmd_len
          << " ,engine_id " << (int)hdr->engine_id;
    }
    i += cmd_len;
  }
}
#endif /* if defined(CHIP) && CHIP == BM1880v2*/

int main (int argc, char *argv[])
{
#if defined(CHIP) && CHIP == BM1880v2
  char* layer_id_name_mapping_file_path = NULL;
  char* output_file_path = NULL;
  if (argc != 4 && argc != 2) {
    printf("Usage: %s cmdbuf.bin <layer_id_name_mapping_file_path output_file_path>\n", argv[0]);
    exit(1);
  }

  if (argc == 4) {
    layer_id_name_mapping_file_path = argv[2];
    output_file_path = argv[3];
  }

  read_bm1880v2_cmd(argv[1], layer_id_name_mapping_file_path, output_file_path);
#else
  if (argc != 2) {
    printf("Usage: %s cmdbuf.bin\n", argv[0]);
    exit(1);
  }

  std::fstream f_cmdbuf(argv[1], std::ios::in | std::ios::binary);

  size_t cmdbuf_size = get_file_length(f_cmdbuf);
  printf("cmdbuf size %zu\n", cmdbuf_size);
  uint8_t *cmdbuf = new uint8_t[cmdbuf_size];
  f_cmdbuf.read((char *)cmdbuf, cmdbuf_size);
  //dump_hex("cmdbuf", cmdbuf, 16);

  parse_cmdbuf(cmdbuf, cmdbuf_size);
#endif /* if CHIP == "BM1880v2"*/

  return 0;
}
