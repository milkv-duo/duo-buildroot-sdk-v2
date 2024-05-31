#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <bmkernel/bm1822/bm1822_tiu_reg.h>
#include <bmkernel/bm1822/bm1822_tdma_reg.h>
#include "bmruntime_internal.h"
#include <bmkernel/bm_regcpu.h>
#include <bmkernel/reg_bdcast.h>
#include <bmkernel/reg_tdma.h>


struct TPU_PMU_DOUBLEEVENT {
  unsigned long long type : 4;
  unsigned long long desID : 16;
  unsigned long long eventCnt0 : 22;
  unsigned long long eventCnt1 : 22;
  uint32_t endTime;
  uint32_t startTime;
};

typedef enum _EXCEL_TYPE {
  EXCEL_TYPE_0    = 0,
  EXCEL_TYPE_1    = 1,
  EXCEL_TYPE_2    = 2,
  EXCEL_TYPE_3    = 3,
  EXCEL_TYPE_4    = 4,
} EXCEL_TYPE;

enum TPU_PMUTYPE {
  TPU_PMUTYPE_TDMALOAD  = 1,
  TPU_PMUTYPE_TDMASTORE = 2,
  TPU_PMUTYPE_TDMAMOVE  = 3,
  TPU_PMUTYPE_TIU       = 4,
};

typedef struct _TPU_DES_ELEMENT {
  TPU_PMU_DOUBLEEVENT pmuEvent;
  tiu_reg_t   tiuReg;
  tdma_reg_t  tdmaReg;
  char typeStr[50];
} TPU_DES_ELEMENT;

typedef struct _TPU_LAYERID_ELEMENT {
  uint32_t    layerID;
  TPU_PMUTYPE last_desType;
  uint32_t    last_mapping_desID;
  uint32_t    endTime;
  uint32_t    startTime;
//  uint8_t     layerName[50];
  uint32_t    u32StartAddr;
  uint32_t    u32OutputLen;

  uint32_t    u32LoadNueronTime;
  uint32_t    u32LoadWeightTime;
  uint32_t    u32StoreNueronTime;
  uint32_t    u32TIUTime;
  uint32_t    u32TDMATime;
  uint32_t    u32byteCnt;

  double      parallelism;
  double      duration_percent;
  double      loadNeuron_percent;
  double      loadWeight_percent;
  double      storeNeuron_percent;
  double      tiu_percent;
  double      throughput_MB;
} TPU_LAYERID_ELEMENT;

#define FILE_OUT_LINE_LEN 2048
#define TPUPMU_DES_FILENAME "_des.csv"
#define TPUPMU_LAYER_FILENAME "_layer.csv"
const char *pmubuf_output_file_env = NULL;


#define TPU_CLOCK_DEFAULT (750000000)
#define TPU_WRAP_LIMIT  0xFFFFFFFF
#define TPU_BURST_SIZE  16
#define DES_MAX   (65535 * 6)    //hardcore firstly, real count could be queried from dmabuf
TPU_DES_ELEMENT *p_element = NULL;
TPU_LAYERID_ELEMENT *p_layer = NULL;

static void tpu_pmu_fill_cmdbuf(uint8_t *v_dma_buf);

static void reorder_back_tiu_cmdbuf_reg(uint8_t *cmdbuf)
{
  int total_bits = BD_REG_BYTES * 8;

  uint8_t tmp[128 / 8];
  uint8_t *last = &cmdbuf[(total_bits - 128) / 8];
  memcpy(tmp, last, sizeof(tmp));
  memcpy(last, cmdbuf, sizeof(tmp));
  memcpy(cmdbuf, tmp, sizeof(tmp));
}

static void tdma_des_fill_str(TPU_DES_ELEMENT *element)
{
  char str1[50] = {0};
  char tmpStr[10] = {0};

  switch(element->pmuEvent.type) {
    case TPU_PMUTYPE_TDMALOAD:
      sprintf(tmpStr, "%s", "Load");
      break;
    case TPU_PMUTYPE_TDMASTORE:
      sprintf(tmpStr, "%s", "Store");
      break;
    case TPU_PMUTYPE_TDMAMOVE:
      sprintf(tmpStr, "%s", "Move");
      break;
    default:
      break;
  }

  if (element->tdmaReg.compress_en)
    sprintf(str1 , "%s %s", tmpStr , "Compression");
  else
    sprintf(str1 , "%s" , tmpStr);

  if (element->tdmaReg.sys_dtype)
    sprintf(element->typeStr, "%s %s", "TDMA Matrix", str1);
  else
    sprintf(element->typeStr, "%s %s", "TDMA Tensor", str1);
}

static void tpu_pmu_fill_cmdbuf(uint8_t *v_dma_buf)
{
  cvi_cpu_desc_t *desc = (cvi_cpu_desc_t *)(v_dma_buf + sizeof(dma_hdr_t));

  uint64_t tiu_offset = 0, tdma_offset = 0;
  uint32_t tiu_cnt = 0, tdma_cnt = 0, i = 0, offset = 0;
  uint32_t start_index_tdma = 0, start_index_tiu = 0;
  uint32_t index = 0;
  tdma_reg_t tmpTDMA_Reg;
  tiu_reg_t tmpTIU_Reg;
  uint8_t tiu_recorded_buf[BD_REG_BYTES];
  uint32_t tdma_id_previous = 0, tdma_start_pre= 0, tdma_end_pre = 0;

  //get tiu/tdma descriptor start address
  tiu_offset = desc->offset_tiu_ori_bk;
  tdma_offset = desc->offset_tdma_ori_bk;
  TPU_LOG_DEBUG("tpu_pmu_fill_cmdbuf() tiu_offset=0x%" PRIx64", tdma_offset=0x%" PRIx64 "\n", tiu_offset, tdma_offset);

  tiu_cnt = desc->num_tiu;
  tdma_cnt = desc->num_tdma;
  TPU_LOG_DEBUG("tpu_pmu_fill_cmdbuf() tiu_cnt=%d, tdma_cnt=%d\n", tiu_cnt, tdma_cnt);

  while (p_element[index].pmuEvent.type) {
    if (p_element[index].pmuEvent.type != TPU_PMUTYPE_TIU) {    //tdma

      if ((p_element[index].pmuEvent.desID != tdma_id_previous) ||
          (p_element[index].pmuEvent.startTime != tdma_start_pre) ||
          (p_element[index].pmuEvent.endTime != tdma_end_pre)) {
        for (i = start_index_tdma; i < tdma_cnt; i ++) {
          offset = tdma_offset + ((1 << TDMA_DESCRIPTOR_ALIGNED_BIT) * i);
          parse_tdma_reg(&tmpTDMA_Reg, (uint32_t *)(v_dma_buf + offset));

          if (p_element[index].pmuEvent.desID == tmpTDMA_Reg.cmd_id) {
            memcpy(&p_element[index].tdmaReg, &tmpTDMA_Reg, sizeof(tmpTDMA_Reg));
            tdma_des_fill_str(&p_element[index]);
            start_index_tdma = i + 1;
            tdma_id_previous = p_element[index].pmuEvent.desID;
            tdma_start_pre = p_element[index].pmuEvent.startTime;
            tdma_end_pre = p_element[index].pmuEvent.endTime;
            break;
          }
        }
      } else {  //tdma g2g case, copy 1st to 2nd tdma descriptor
        memcpy(&p_element[index].tdmaReg, &p_element[index - 1].tdmaReg, sizeof(tmpTDMA_Reg));
        tdma_des_fill_str(&p_element[index]);
      }
    } else {   //tiu
      for (i = start_index_tiu; i < tiu_cnt; i ++) {
        offset = tiu_offset + (BD_REG_BYTES * i);
        uint8_t *tiu_cmdbuf = v_dma_buf + offset;

        //get tiu_reg struc
        memcpy(tiu_recorded_buf, tiu_cmdbuf, BD_REG_BYTES);
        reorder_back_tiu_cmdbuf_reg(tiu_recorded_buf);
        parse_tiu_reg(&tmpTIU_Reg, (uint32_t *)tiu_recorded_buf);

        if (p_element[index].pmuEvent.desID == tmpTIU_Reg.cmd_id_tpu) {
          memcpy(&p_element[index].tiuReg, &tmpTIU_Reg, sizeof(tmpTIU_Reg));

#if 1
          switch (tmpTIU_Reg.tsk_typ) {
            case DCR_TYPE_CONV_FIX8B:
              if (!tmpTIU_Reg.opt_chl_quan) {
                if (tmpTIU_Reg.opd_typ) {
                  strcpy(p_element[index].typeStr, "TIU BF16 Convolution");
                } else {
                  strcpy(p_element[index].typeStr, "TIU Convolution");
                }
              } else {
                strcpy(p_element[index].typeStr, "TIU PerChannel Convolution");
              }
              break;
            case DCR_TYPE_DEPTHWISE_POOL_FIX8B:
              switch (tmpTIU_Reg.tsk_eu_typ) {
                case 0:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Max Pooling");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Max Pooling");
                  }
                  break;
                case 1:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Average Pooling");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Average Pooling");
                  }
                  break;
                case 2:
                  if (!tmpTIU_Reg.opt_chl_quan) {
                    if (tmpTIU_Reg.opd_typ) {
                      strcpy(p_element[index].typeStr, "TIU BF16 Depthwise Convolution");
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Depthwise Convolution");
                    }
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Depthwise PerChannel Convolution");
                  }
                  break;
                case 3:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Min Pooling");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Min Pooling");
                  }
                  break;
                default:
                  break;
              }
              break;
            case DCR_TYPE_FC_FIX8B:
              if (!tmpTIU_Reg.opt_chl_quan) {
                if (tmpTIU_Reg.opd_typ) {
                  strcpy(p_element[index].typeStr, "TIU BF16 Matrix Multiplication");
                } else {
                  strcpy(p_element[index].typeStr, "TIU Matrix Multiplication");
                }
              } else {
                strcpy(p_element[index].typeStr, "TIU PerChannel Matrix Multiplication");
              }
              break;
            case DCR_TYPE_TENSOR_ARITH_FIX8B:
              switch(tmpTIU_Reg.tsk_eu_typ) {
                case 0:
                  if (!tmpTIU_Reg.opt_chl_quan) {
                    if (tmpTIU_Reg.opd_typ) {
                       strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Mul");
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Element-wise Mul");
                    }
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Element-wise Mul(QDM)");
                  }
                  break;
                case 1:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Mac");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Element-wise Mac");
                  }
                  break;
                case 2:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Add");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Element-wise Add");
                  }
                  break;
                case 3:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Sub");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Element-wise Sub");
                  }
                  break;
                case 4:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Max");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Element-wise Max");
                  }
                  break;
                case 5:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Min");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Element-wise Min");
                  }
                  break;
                case 6:
                  strcpy(p_element[index].typeStr, "TIU Element-wise Shift");
                  break;
                case 7:
                  strcpy(p_element[index].typeStr, "TIU Element-wise AND");
                  break;
                case 8:
                  strcpy(p_element[index].typeStr, "TIU Element-wise OR");
                  break;
                case 9:
                  strcpy(p_element[index].typeStr, "TIU Element-wise XOR");
                  break;
                case 10:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Copy");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Element-wise Copy");
                  }
                  break;
                case 11:
                  if (tmpTIU_Reg.opd_typ) {
                    strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Ge");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Element-wise Ge");
                  }
                  break;
                case 12:
                  strcpy(p_element[index].typeStr, "TIU Lookup Table");
                  break;
                default:
                  break;
                }
                default:
                  break;
          }

#else
          switch(tmpTIU_Reg.tsk_typ) {
            case DCR_TYPE_CONV_FIX8B:
              if (!tmpTIU_Reg.opt_chl_quan) {
                if (tmpTIU_Reg.opd_typ)
                  strcpy(p_element[index].typeStr, "TIU BF16 Convolution");
                else
                  strcpy(p_element[index].typeStr, "TIU Convolution");
              } else {
                strcpy(p_element[index].typeStr, "TIU PerChannel Convolution");
              }
              break;
            case DCR_TYPE_DEPTHWISE_POOL_FIX8B:
              switch (tmpTIU_Reg.tsk_eu_typ) {
                    case 0:
                  if (tmpTIU_Reg.opd_typ)
                    strcpy(p_element[index].typeStr, "TIU BF16 Max Pooling");
                  else
                    strcpy(p_element[index].typeStr, "TIU Max Pooling");
                  break;
                    case 1:
                  if (tmpTIU_Reg.opd_typ)
                    strcpy(p_element[index].typeStr, "TIU BF16 Average Pooling");
                  else
                    strcpy(p_element[index].typeStr, "TIU Average Pooling");
                  break;
                    case 2:
                  if (!tmpTIU_Reg.opt_chl_quan) {
                    if (tmpTIU_Reg.opd_typ)
                      strcpy(p_element[index].typeStr, "TIU BF16 Depthwise Convolution");
                    else
                      strcpy(p_element[index].typeStr, "TIU Depthwise Convolution");
                  } else {
                    strcpy(p_element[index].typeStr, "TIU Depthwise PerChannel Convolution");
                  }
                  break;
                    default:
                      break;
                  }
              break;
            case DCR_TYPE_FC_FIX8B:
              if (!tmpTIU_Reg.opt_chl_quan) {
                if (tmpTIU_Reg.opd_typ)
                  strcpy(p_element[index].typeStr, "TIU BF16 Matrix Multiplication");
                else
                  strcpy(p_element[index].typeStr, "TIU Matrix Multiplication");
              } else {
                strcpy(p_element[index].typeStr, "TIU PerChannel Matrix Multiplication");
              }
              break;
            case DCR_TYPE_TENSOR_ARITH_FIX8B:
              if (tmpTIU_Reg.tens_mdsum) {
                strcpy(p_element[index].typeStr, "TIU Mdsum");
              } else if (tmpTIU_Reg.tens_lookup) {
                strcpy(p_element[index].typeStr, "TIU Lookup Table");
              } else {
                switch (tmpTIU_Reg.tsk_eu_typ) {
                  case 0:
                    if (!tmpTIU_Reg.opt_chl_quan) {
                      if (tmpTIU_Reg.opd_typ) {
                        strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Mul");
                      } else {
                        strcpy(p_element[index].typeStr, "TIU Element-wise Mul");
                      }
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Element-wise Mul(QDM)");
                    }
                    break;
                  case 1:
                    if (tmpTIU_Reg.opd_typ) {
                      strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Mac");
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Element-wise Mac");
                    }
                    break;
                  case 2:
                    if (tmpTIU_Reg.opd_typ) {
                      strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Add");
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Element-wise Add");
                    }
                    break;
                  case 3:
                    if (tmpTIU_Reg.opd_typ) {
                      strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Sub");
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Element-wise Sub");
                    }
                    break;
                  case 4:
                    if (tmpTIU_Reg.opd_typ) {
                      strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Max");
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Element-wise Max");
                    }
                    break;
                  case 5:
                    if (tmpTIU_Reg.opd_typ) {
                      strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Min");
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Element-wise Min");
                    }
                    break;
                  case 6:
                    strcpy(p_element[index].typeStr, "TIU Element-wise Shift");
                    break;
                  case 7:
                    strcpy(p_element[index].typeStr, "TIU Element-wise AND");
                    break;
                  case 8:
                    strcpy(p_element[index].typeStr, "TIU Element-wise OR");
                    break;
                  case 9:
                    strcpy(p_element[index].typeStr, "TIU Element-wise XOR");
                    break;
                  case 10:
                    if (tmpTIU_Reg.opd_typ) {
                      strcpy(p_element[index].typeStr, "TIU BF16 Element-wise Copy");
                    } else {
                      strcpy(p_element[index].typeStr, "TIU Element-wise Copy");
                    }
                    break;
                  default:
                    break;
                }
              }
              break;
          }
#endif
          start_index_tiu = i + 1;
          break;
        }
      }
    }
    index ++;
  }

}

#include <iostream>
#include <fstream>
using namespace std;

static void tpu_pmu_fwrite_des()
{
  uint32_t index = 0;
	uint64_t srcAddr = 0, dstAddr = 0;

  char lineStr[FILE_OUT_LINE_LEN] = {0};
  EXCEL_TYPE excelType = EXCEL_TYPE_0;

  std::fstream fout_element;
  sprintf(lineStr, "%s%s", pmubuf_output_file_env, TPUPMU_DES_FILENAME);
  TPU_LOG_DEBUG("out file_des name=%s\n", lineStr);
  fout_element.open(lineStr, std::ios::out | std::ios::trunc);

  strcpy(lineStr, "pmutype, desID, event0, event1, , start, duration, end, layerID, desType, \
    srcAddr, dstAddr, trans_fmt, transpose_md, cmd_id, wait_id_tpu, dst_h_stride, dst_c_stride_low, \
    dst_n_stride, src_h_stride, src_c_stride_low, src_n_stride, dst_c, src_c, dst_w, dst_h, src_w, src_h, src_n\n");
  fout_element << lineStr;

  //dump descriptor content related
  while (p_element[index].pmuEvent.type)
  {
    switch (p_element[index].pmuEvent.type) {
      case TPU_PMUTYPE_TDMALOAD:
        excelType = EXCEL_TYPE_1;
        break;
      case TPU_PMUTYPE_TDMASTORE:
      case TPU_PMUTYPE_TDMAMOVE:
        excelType = EXCEL_TYPE_2;
        break;
      case TPU_PMUTYPE_TIU:
        excelType = EXCEL_TYPE_3;
        break;
    }

    if (p_element[index].pmuEvent.type == TPU_PMUTYPE_TIU) {
      sprintf(lineStr, "%u, %u, %u, %u, %u, %u, %u, %u, %u, %s\n",
                        p_element[index].pmuEvent.type,
                        p_element[index].pmuEvent.desID,
                        p_element[index].pmuEvent.eventCnt0,
                        p_element[index].pmuEvent.eventCnt1,
                        excelType,
                        p_element[index].pmuEvent.startTime,
                        p_element[index].pmuEvent.endTime - p_element[index].pmuEvent.startTime,
                        p_element[index].pmuEvent.endTime,
                        p_element[index].tiuReg.layer_info,
                        p_element[index].typeStr);
    } else {
      srcAddr = ((uint64_t)(p_element[index].tdmaReg.src_base_addr_high) << 32) |
                  (uint64_t)(p_element[index].tdmaReg.src_base_addr_low);
      dstAddr = ((uint64_t)(p_element[index].tdmaReg.dst_base_addr_high) << 32) |
                  (uint64_t)(p_element[index].tdmaReg.dst_base_addr_low);

      sprintf(lineStr, "%u, %u, %u, %u, %u, %u, %u, %u, %u, %s, 0x%" PRIu64 ", 0x%" PRIu64 ", \
        %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u\n",
                        p_element[index].pmuEvent.type,
                        p_element[index].pmuEvent.desID,
                        p_element[index].pmuEvent.eventCnt0,
                        p_element[index].pmuEvent.eventCnt1,
                        excelType,
                        p_element[index].pmuEvent.startTime,
                        p_element[index].pmuEvent.endTime - p_element[index].pmuEvent.startTime,
                        p_element[index].pmuEvent.endTime,
                        p_element[index].tdmaReg.layer_ID,
                        p_element[index].typeStr,
                        srcAddr,
                        dstAddr,
                        p_element[index].tdmaReg.trans_fmt,
                        p_element[index].tdmaReg.transpose_md,
                        p_element[index].tdmaReg.cmd_id,
                        p_element[index].tdmaReg.wait_id_tpu,
                        p_element[index].tdmaReg.dst_h_stride,
                        p_element[index].tdmaReg.dst_c_stride_low,
                        p_element[index].tdmaReg.dst_n_stride,
                        p_element[index].tdmaReg.src_h_stride,
                        p_element[index].tdmaReg.src_c_stride_low,
                        p_element[index].tdmaReg.src_n_stride,
                        p_element[index].tdmaReg.dst_c,
                        p_element[index].tdmaReg.src_c,
                        p_element[index].tdmaReg.dst_w,
                        p_element[index].tdmaReg.dst_h,
                        p_element[index].tdmaReg.src_w,
                        p_element[index].tdmaReg.src_h,
                        p_element[index].tdmaReg.src_n);
    }

    fout_element << lineStr;
    index ++;
  }

  fout_element.close();
}

static void tpu_pmu_getlayerInfo(void)
{
  uint32_t index = 0, layIDIndex = 0;
  uint32_t curLayID = 0;
  uint32_t u32SingleDuration = 0;

  TPU_LOG_DEBUG("tpu_pmu_getlayerInfo() start\n");
  while (p_element[index].pmuEvent.type) {
    if (!curLayID) {
      //record current layerID
      curLayID = p_element[index].pmuEvent.type == TPU_PMUTYPE_TIU ?
      p_element[index].tiuReg.layer_info : p_element[index].tdmaReg.layer_ID;

      p_layer[layIDIndex].last_desType = (TPU_PMUTYPE)p_element[index].pmuEvent.type;
      p_layer[layIDIndex].layerID = curLayID;
      p_layer[layIDIndex].endTime = p_element[index].pmuEvent.endTime;
      p_layer[layIDIndex].startTime = p_element[index].pmuEvent.startTime;
      p_layer[layIDIndex].last_mapping_desID = p_element[index].pmuEvent.desID;
    } else {
      //if next layer ID is identical
      if (curLayID == (p_element[index].pmuEvent.type == TPU_PMUTYPE_TIU ?
        p_element[index].tiuReg.layer_info : p_element[index].tdmaReg.layer_ID)) {
        p_layer[layIDIndex].endTime = (p_element[index].pmuEvent.endTime > p_layer[layIDIndex].endTime) ?
          (p_element[index].pmuEvent.endTime) : (p_layer[layIDIndex].endTime);

        p_layer[layIDIndex].last_mapping_desID = p_element[index].pmuEvent.desID;

      } else {
        layIDIndex ++;
        curLayID = p_element[index].pmuEvent.type == TPU_PMUTYPE_TIU ?
          p_element[index].tiuReg.layer_info : p_element[index].tdmaReg.layer_ID;

        p_layer[layIDIndex].last_desType = (TPU_PMUTYPE)p_element[index].pmuEvent.type;
        p_layer[layIDIndex].layerID = curLayID;
        p_layer[layIDIndex].endTime = p_element[index].pmuEvent.endTime;
        p_layer[layIDIndex].startTime = p_element[index].pmuEvent.startTime;
        p_layer[layIDIndex].last_mapping_desID = p_element[index].pmuEvent.desID;
      }
    }

    //get each duration and then classfy by type
    u32SingleDuration = p_element[index].pmuEvent.endTime - p_element[index].pmuEvent.startTime;
    switch (p_element[index].pmuEvent.type) {
      case TPU_PMUTYPE_TIU:
        p_layer[layIDIndex].u32TIUTime += u32SingleDuration;
        break;

      case TPU_PMUTYPE_TDMALOAD:
        if (p_element[index].tdmaReg.src_base_reg_sel == 0)
          p_layer[layIDIndex].u32LoadNueronTime += u32SingleDuration;
        else if (p_element[index].tdmaReg.src_base_reg_sel == 1)
          p_layer[layIDIndex].u32LoadWeightTime += u32SingleDuration;

        p_layer[layIDIndex].u32TDMATime += u32SingleDuration;
        break;

      case TPU_PMUTYPE_TDMASTORE:
        if (p_element[index].tdmaReg.src_base_reg_sel == 0)
          p_layer[layIDIndex].u32StoreNueronTime += u32SingleDuration;

        p_layer[layIDIndex].u32TDMATime += u32SingleDuration;
        break;

      default:
        break;
    }

    //accumulate byte counts, one burst count = 16bytes
    p_layer[layIDIndex].u32byteCnt += (p_element[index].pmuEvent.eventCnt1 * 16);
    index ++;
  }
}

static void tpu_pmu_fwrite_layer(uint64_t tpu_clock)
{
  uint32_t index = 0;
  char lineStr[FILE_OUT_LINE_LEN] = {0};
  uint64_t u64totalDuration = 0, u64singleDuration = 0;
  std::fstream fout_layer;

  sprintf(lineStr, "%s%s", pmubuf_output_file_env, TPUPMU_LAYER_FILENAME);
  TPU_LOG_DEBUG("out file_des name=%s\n", lineStr);
  fout_layer.open(lineStr, std::ios::out | std::ios::trunc);

  //pre-processing once, and we can get total duration
  index = 0;
  while (p_layer[index].layerID) {
    u64totalDuration += p_layer[index].endTime - p_layer[index].startTime;
    index ++;
  }

  index = 0;
  while (p_layer[index].layerID) {
    u64singleDuration = p_layer[index].endTime - p_layer[index].startTime;
    p_layer[index].parallelism = (double)(p_layer[index].u32TDMATime + p_layer[index].u32TIUTime) / (double)u64singleDuration * 100;
    p_layer[index].parallelism =  p_layer[index].parallelism < 100 ? 100 : p_layer[index].parallelism;

    p_layer[index].duration_percent = (double)u64singleDuration / (double)u64totalDuration * 100;
    p_layer[index].tiu_percent = (double)p_layer[index].u32TIUTime / (double)u64singleDuration * 100;
    p_layer[index].loadNeuron_percent = (double)p_layer[index].u32LoadNueronTime / (double)u64singleDuration * 100;
    p_layer[index].loadWeight_percent = (double)p_layer[index].u32LoadWeightTime / (double)u64singleDuration * 100;
    p_layer[index].storeNeuron_percent = (double)p_layer[index].u32StoreNueronTime / (double)u64singleDuration * 100;
    p_layer[index].throughput_MB = (double)p_layer[index].u32byteCnt * tpu_clock / (double)u64singleDuration / 1024 / 1024;
    index ++;
  }

  strcpy(lineStr, "layerID, start, duration, end, duration(%), parallelism(%), TIU(%), \
    loadNeuron(%), loadWeight(%), storeNeuron(%), throughput(MB/s), last_tdmaID, dumpStart, dumpLen, TIU, loadNeuron, \
    loadWeight, storeNeuron, byteCnt\n");

  fout_layer << lineStr;

  index = 0;
  while (p_layer[index].layerID) {
    sprintf(lineStr, "%d, %d, %d, %d, %lf%%, %lf%%, %lf%%, %lf%%, %lf%%, %lf%%, %.2lfMB/s, %d, 0x%x, 0x%x, %d, %d, %d, %d, %d\n",
                p_layer[index].layerID,
                p_layer[index].startTime,
                p_layer[index].endTime - p_layer[index].startTime,
                p_layer[index].endTime,

                p_layer[index].duration_percent,
                p_layer[index].parallelism,
                p_layer[index].tiu_percent,
                p_layer[index].loadNeuron_percent,
                p_layer[index].loadWeight_percent,
                p_layer[index].storeNeuron_percent,
                p_layer[index].throughput_MB,

                p_layer[index].last_mapping_desID,
                p_layer[index].u32StartAddr,
                p_layer[index].u32OutputLen,
                p_layer[index].u32TIUTime,
                p_layer[index].u32LoadNueronTime,
                p_layer[index].u32LoadWeightTime,
                p_layer[index].u32StoreNueronTime,
                p_layer[index].u32byteCnt);
    fout_layer << lineStr;
    index ++;
  }

  fout_layer.close();
}

static int tpu_pmu_time(uint8_t *v_dma_buf, uint64_t p_dma_buf, uint8_t all_info)
{
  dma_hdr_t *header = (dma_hdr_t *)(v_dma_buf);
  struct TPU_PMU_DOUBLEEVENT *pCurrent = (struct TPU_PMU_DOUBLEEVENT *)(v_dma_buf + header->pmubuf_offset);

  uint64_t bmnet_p_total = 0;
  uint64_t bmnet_p_duration = 0;

  uint64_t u64TDMATotal = 0;
  uint64_t u64TIUTotal = 0;
  uint64_t u64_des_start = 0, u64_des_end = 0;
  uint32_t u32TDMACnt = 0, u32TIUCnt = 0;
  uint32_t index = 0, diff = 0, wrap_cnt = 0;
  uint32_t tpu_clk_rate = header->tpu_clk_rate;
  uint64_t u64_load_bytes = 0, u64_store_bytes = 0;
  uint32_t tdma_id_previous = 0, tdma_start_pre= 0, tdma_end_pre = 0;
  double percent_tdma = 0, percent_tiu = 0, percent_paralellism = 0;
  double ms_tdma = 0, ms_tiu = 0, ms_influence = 0;
  double load_mb = 0, store_mb = 0;
  double bandwidth = 0;

  TPU_LOG_DEBUG("TPU_LOG_DEBUG tpu_pmu_time() all_info=%x\n", all_info);
  //traverse pmu buffer
  while (*(uint32_t *)pCurrent) {
    if (pCurrent->type >= TPU_PMUTYPE_TDMALOAD && pCurrent->type <= TPU_PMUTYPE_TIU) {
      if (index == 0) {
        u64_des_start = pCurrent->startTime;
        u64_des_end = pCurrent->endTime;
      } else {
        u64_des_end = pCurrent->endTime;
      }

      if (all_info)
        memcpy(&p_element[index].pmuEvent, pCurrent, sizeof(TPU_PMU_DOUBLEEVENT));

    } else {
      TPU_LOG_ERROR("pmubuf content header type incorrect, just next\n");
      index ++;
      pCurrent++;
      continue;
    }

    if (pCurrent->type == TPU_PMUTYPE_TIU) {  //tiu case
      if (pCurrent->endTime > pCurrent->startTime) {
        diff = pCurrent->endTime - pCurrent->startTime;
      } else {
        diff = 0xFFFFFFFF - pCurrent->startTime + pCurrent->endTime;
        wrap_cnt ++;
      }

      u64TIUTotal += diff;
      u32TIUCnt++;
    } else {    //tdma case

      //g2g will generate two des loadx1+storex1, we only accumulate one of them
      if ((pCurrent->desID != tdma_id_previous) ||
          (pCurrent->startTime != tdma_start_pre) ||
          (pCurrent->endTime != tdma_end_pre)) {

        if (pCurrent->endTime > pCurrent->startTime) {
          diff = pCurrent->endTime - pCurrent->startTime;
        } else {
          diff = TPU_WRAP_LIMIT - pCurrent->startTime + pCurrent->endTime;
          wrap_cnt ++;
        }
        u64TDMATotal += diff;
        u32TDMACnt++;
      }

      if (pCurrent->type == TPU_PMUTYPE_TDMALOAD) {
        u64_load_bytes += TPU_BURST_SIZE * pCurrent->eventCnt1;
      } else if (pCurrent->type == TPU_PMUTYPE_TDMASTORE) {
        u64_store_bytes += TPU_BURST_SIZE * pCurrent->eventCnt1;
      }

      tdma_id_previous = pCurrent->desID;
      tdma_start_pre = pCurrent->startTime;
      tdma_end_pre = pCurrent->endTime;
    } 

    index ++;
    pCurrent++;
  }

  bmnet_p_total = u64TDMATotal + u64TIUTotal;
  if (wrap_cnt)
    bmnet_p_duration = TPU_WRAP_LIMIT * (wrap_cnt - 1) + TPU_WRAP_LIMIT - u64_des_start + u64_des_end;
  else
    bmnet_p_duration = u64_des_end - u64_des_start;

  percent_tdma = (double)u64TDMATotal / (double)bmnet_p_duration * (double)100;
  percent_tiu = (double)u64TIUTotal / (double)bmnet_p_duration * (double)100;
  percent_paralellism = (double)(bmnet_p_total) / (double)bmnet_p_duration * (double)100;
  percent_paralellism = percent_paralellism < 100 ? 100 : percent_paralellism;

  if (!tpu_clk_rate) {
    tpu_clk_rate = TPU_CLOCK_DEFAULT;
    printf("can't get tpu clock, assume to %dMhz\n", tpu_clk_rate / 1000000);
  }

  ms_tdma = (double)u64TDMATotal / (double)tpu_clk_rate * (double)1000;
  ms_tiu = (double)u64TIUTotal / (double)tpu_clk_rate * (double)1000;
  ms_influence = (double)bmnet_p_duration / (double)tpu_clk_rate * (double)1000;

  load_mb =  (double)u64_load_bytes / (double)1024 / (double)1024;
  store_mb =  (double)u64_store_bytes / (double)1024 / (double)1024;

  bandwidth = (double)(load_mb + store_mb) / (double)ms_influence * (double)1000;

  printf("=======================inference total info ==========================\n");
  //printf("cv183x tpu clock: %dMhz\n", header->tpu_clk_rate / 1000000);
  printf("%-20s %8dMhz, %-20s %9.2fMB, %-20s %7.2fMB/s\n",
          "cv180x_tpu_clock:", tpu_clk_rate / 1000000, "inferece_data:", load_mb + store_mb, "inference_bw:", bandwidth);

  printf("%-20s %10" PRIu64 "t, %-20s %10" PRIu64 "t, %-20s %10" PRIu64 "t\n",
         "tdma_exe_tick:", u64TDMATotal, "tiu_exe_tick", u64TIUTotal, "inference_tick", bmnet_p_duration);
  printf("%-20s %10.2f%%, %-20s %10.2f%%, %-20s %10.2f%%\n",
          "tdma_exe_percent:", percent_tdma, "tiu_exe_percent:", percent_tiu, "paralellism_percent", percent_paralellism);
  printf("%-20s %9.2fms, %-20s %9.2fms, %-20s %9.2fms\n",
          "tdma_exe_ms:", ms_tdma, "tiu_exe_ms:", ms_tiu, "inference_ms:", ms_influence);

  if (all_info) {
    tpu_pmu_fill_cmdbuf(v_dma_buf);
    tpu_pmu_fwrite_des();
    tpu_pmu_getlayerInfo();
    tpu_pmu_fwrite_layer(tpu_clk_rate);
  }

  return 0;
}

uint32_t tpu_pmu_get_des_cnt(uint8_t *v_dma_buf)
{
  uint32_t tiu_cnt = 0, tdma_cnt = 0;
  dma_hdr_t *header = (dma_hdr_t *)v_dma_buf;
  cvi_cpu_desc_t *desc = (cvi_cpu_desc_t *)(v_dma_buf + sizeof(dma_hdr_t));

  for (uint32_t i = 0; i < header->cpu_desc_count; i++, desc++) {
    tiu_cnt += (desc->num_tiu & 0xFFFF);
    tdma_cnt += (desc->num_tdma & 0xFFFF);
  }

  //assume worst case tdma g2g case will generate double descriptor
  return (tiu_cnt + tdma_cnt + tdma_cnt);
}

#define TPU_PMU_MALLOC_PADDING  1024
uint32_t tpu_pmu_dump_main(uint8_t *v_dma_buf, uint64_t p_dma_buf)
{
  dma_hdr_t *dma_header = (dma_hdr_t *)v_dma_buf;
  uint8_t all_info = 0;

  //check header first
  if (dma_header->dmabuf_magic_m != TPU_DMABUF_HEADER_M) {
    TPU_LOG_NOTICE("pmu buffer header incorrect\n");
    return CVI_RC_FAILURE;
  }

  //check if we need output pmubuf
  pmubuf_output_file_env = std::getenv("TPU_PMUBUF_OUTPUT_FILE");
  if (pmubuf_output_file_env) {
    all_info = 1;
  }

  //malloc element array
  if (all_info) {
    p_element = (TPU_DES_ELEMENT *)malloc(tpu_pmu_get_des_cnt(v_dma_buf) * sizeof(TPU_DES_ELEMENT) + TPU_PMU_MALLOC_PADDING);
    p_layer = (TPU_LAYERID_ELEMENT *)malloc(tpu_pmu_get_des_cnt(v_dma_buf) * sizeof(TPU_LAYERID_ELEMENT) + TPU_PMU_MALLOC_PADDING);

    if (!p_element || !p_layer) {
      TPU_LOG_INFO("tpu pmu des array malloc failed\n");
      return CVI_RC_FAILURE;
    }
  }

  //get pmu overview data
  tpu_pmu_time(v_dma_buf, p_dma_buf, all_info);

  //free element array
  if (all_info) {
    if (p_element) {
      free(p_element);
      p_element = NULL;
    }

    if (p_layer) {
      free(p_layer);
      p_layer = NULL;
    }
  }

  return CVI_RC_SUCCESS;
}

