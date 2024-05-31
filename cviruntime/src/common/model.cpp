#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <mutex>
#include <runtime/model.hpp>
#include <runtime/stream.hpp>
#include <runtime/debug.h>
#include <runtime/version.h>
#include <alloc.h>

#ifdef ENABLE_CPU_FUNC
#include <cpu_function/deformableconv.hpp>
#include <cpu_function/deform_im2col.hpp>
#include <cpu_function/instancenorm.hpp>
#include <cpu_function/interpolation.hpp>
#include <cpu_function/softmax.hpp>
#include <cpu_function/quant.hpp>
#include <cpu_function/reducemean.hpp>
#include <cpu_function/reducemax.hpp>
#include <cpu_function/reducel2.hpp>
#include <cpu_function/retinaface_detection.hpp>
#include <cpu_function/preprocess.hpp>
#include <cpu_function/transpose.hpp>
#include <cpu_function/ssd_detection.hpp>
#include <cpu_function/yolo_detection.hpp>
#include <cpu_function/frcn_detection.hpp>
#include <cpu_function/pixelshuffle.hpp>
#include <cpu_function/proposal.hpp>
#include <cpu_function/roi_pooling.hpp>
#include <cpu_function/argmax.hpp>
#include <cpu_function/argmax_v2.hpp>
#include <cpu_function/argmax_v3.hpp>
#include <cpu_function/embedding.hpp>
#include <cpu_function/gathernd.hpp>
#include <cpu_function/grid_sampler.hpp>
#include <cpu_function/cumsum.hpp>
#include <cpu_function/gatherelements_pt.hpp>
#endif

#ifdef ENABLE_COMPRESS_CMDBUF
#include "lz4.h"
#endif

namespace cvi {
namespace runtime {

std::string CviModel::targetChipType = "";

CviModel::CviModel(CVI_RT_HANDLE ctx, int count)
    : _ctx(ctx), ref(1), _count(count), _max_shared_mem_size(0) {
  _pool = new TaskPool(1);
  if (std::getenv("TPU_ENABLE_PROTECT")) {
    isprotect = true;
  }

#ifdef ENABLE_CPU_FUNC
  _cpu_functions.push_back(new CpuRuntimeFunction("deform_conv2d", DeformableConvFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("deform_im2col", DeformableIm2ColFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("instance_norm", InstanceNormFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("interp", InterpolationFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("softmax", SoftmaxFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("softmax_cpu", SoftmaxFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("quant", QuantFunc::open));
  _cpu_functions.push_back(
      new CpuRuntimeFunction("retinaface_detection", RetinaFaceDetectionFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("preprocess", PreprocessFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("transpose", TransposeFunc::open));
  _cpu_functions.push_back(
      new CpuRuntimeFunction("detectionoutput", SSDDetectionFunc::open));
  _cpu_functions.push_back(
      new CpuRuntimeFunction("yolo_detection", YoloDetectionFunc::open));
  _cpu_functions.push_back(
      new CpuRuntimeFunction("frcn_detection", FrcnDetectionFunc::open));
  _cpu_functions.push_back(
      new CpuRuntimeFunction("pixelshuffle", PixelShuffleFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("proposal", ProposalFunc::open));
  _cpu_functions.push_back(
      new CpuRuntimeFunction("cpu_reduce_mean", ReduceMeanFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("cpu_reduce_max", ReduceMaxFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("reduce_l2", ReduceL2Func::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("roi_pooling", ROIPoolingFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("argmax", ArgMaxFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("argmax_with_conf", ArgMaxV2Func::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("argmax_v3", ArgMaxV3Func::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("embedding", EmbeddingFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("gathernd_tf", GatherNDFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("grid_sampler", GridSamplerFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("cumsum", CumSumFunc::open));
  _cpu_functions.push_back(new CpuRuntimeFunction("gatherelements_pt", GatherElementsPtFunc::open));
#endif
}

CviModel::~CviModel() {
  if (_model_body)
    delete[] _model_body;
  if (_pool)
    delete _pool;
  if (_weight_mem) {
    if (isprotect) {
      mem_unprotect(CVI_RT_MemGetVAddr(_weight_mem), CVI_RT_MemGetSize(_weight_mem));
    }
    cviMemFree(_ctx, _weight_mem);
  }
  for (auto func : _cpu_functions) {
    delete func;
  }
  for (auto buf : dmabuf_map) {
    if (isprotect) {
      mem_unprotect(CVI_RT_MemGetVAddr(buf.second), CVI_RT_MemGetSize(buf.second));
    }
    cviMemFree(_ctx, buf.second);
  }
}

bool CviModel::checkIfMatchTargetChipType(std::string &target) {
#if defined(__x86_64__) || defined(_M_X64)
  char *deviceChipType = std::getenv("SET_CHIP_NAME");
#else
#if CHIPID == 0x1
  char *deviceChipType = (char *)"cv183x";
#elif CHIPID == 0x2
  char *deviceChipType = (char *)"cv182x";
#elif CHIPID ==  0x3
  char *deviceChipType = (char *)"cv181x";
#elif CHIPID == 0x4
  char *deviceChipType = (char *)"cv180x";
#else
#error "CHIPID is not defined"
#endif
#endif
  if (target != deviceChipType) {
    TPU_LOG_ERROR("cvimodel built for %s CANNOT run on platform %s\n",
                  target.c_str(), deviceChipType);
    return false;
  }
  return true;
}

CVI_RC CviModel::parseModelHeader(BaseStream *stream, size_t &payload_sz, size_t &header_sz) {
  if (stream->length() <= sizeof(MODEL_HEADER)) {
    TPU_LOG_ERROR("Error, invalid cvimodel file\n");
    return CVI_RC_INVALID_ARG;
  }

  MODEL_HEADER header;
  stream->read((uint8_t *)&header, 0, sizeof(header));
  payload_sz = header.body_size;
  /* before version 1.1, heder size is 32 bytes */
  if (header.major == 1 && header.minor == 0) {
    header_sz = 0x20;
  } else {
    header_sz = sizeof(MODEL_HEADER);
  }

  /* No chip field in heder before version 1.1 */
  if (header.major == 1 && header.minor == 0) {
    targetChipType = "cv183x";
  } else {
    targetChipType = header.chip;
  }
  if (!checkIfMatchTargetChipType(targetChipType)) {
    return CVI_RC_INVALID_ARG;
  }
  // TODO, verify md5 here
  return CVI_RC_SUCCESS;
}

CVI_RC CviModel::showAndCheckVersion() {
  auto version = _fb_model->version();
  major_ver = (int)version->major_();
  minor_ver = (int)version->minor_();
  TPU_LOG_INFO("version: %d.%d.%d\n",
               major_ver, minor_ver, (int)version->sub_minor());
  if (_fb_model->target()) {
    TPU_LOG_INFO("%s Build at %s For platform %s\n", _fb_model->name()->str().c_str(),
                 _fb_model->build_time()->str().c_str(), _fb_model->target()->str().c_str());
  } else {
    TPU_LOG_INFO("%s Build at %s For platform cv183x\n", _fb_model->name()->str().c_str(),
                 _fb_model->build_time()->str().c_str());
  }

  /* runtime should compatible with the previous cvimodel
     cvimodel version should smaller than runtime */
  if (cvi::model::MajorVersion_value > major_ver) {
    return CVI_RC_SUCCESS;
  } else if (cvi::model::MinorVersion_value < minor_ver) {
    TPU_LOG_ERROR("cvimodel(%d.%d) is not supported in runtime(%d.%d)\n",
        major_ver, minor_ver,
        (int)cvi::model::MajorVersion_value,
        (int)cvi::model::MinorVersion_value);
    TPU_LOG_ERROR("Please update runtime lib.\n");
    return CVI_RC_INVALID_ARG ;
  }
  return CVI_RC_SUCCESS;
}

void CviModel::parseProgramNum() {
  auto &programs = *_fb_model->programs();
  program_num = programs.size();

  _max_shared_mem_size = 0;
  for (int i = 0; i < program_num; ++i) {
    if (programs[i]->shared_gmem() > _max_shared_mem_size) {
      _max_shared_mem_size = programs[i]->shared_gmem();
    }
  }
  TPU_LOG_INFO("Max SharedMem size:%zu\n", _max_shared_mem_size);
}

CVI_RC CviModel::extractSections(BaseStream *stream, size_t bin_offset) {
  auto &sections = *_fb_model->sections();
  std::vector<const cvi::model::Section*> cmdbuf_sections;
  CVI_RC ret;
  for (auto s : sections) {
#if __aarch64__
    if (s->type() == cvi::model::SectionType_FUNC_AARCH64) {
#else
    if (s->type() == cvi::model::SectionType_FUNC_X86) {
#endif
      if (s->size() == 0)
        continue;
      if (!_custom_section.load(stream, s->offset() + bin_offset, s->size(),
                                _cpu_functions)) {
        return CVI_RC_FAILURE;
      }
    } else if (s->type() == cvi::model::SectionType_WEIGHT) {
      ret = loadWeight(stream, s->offset() + bin_offset, s->size());
      if (ret != CVI_RC_SUCCESS) {
        return ret;
      }
    } else if (s->type() == cvi::model::SectionType_CMDBUF) {
      cmdbuf_sections.emplace_back(s);
    } else if (s->type() == cvi::model::SectionType_DMABUF) {
      ret = loadDmabuf(stream, s->offset() + bin_offset, s->size(), s);
      if (ret != CVI_RC_SUCCESS) {
        return ret;
      }
    }
  }
  for (auto s : cmdbuf_sections) {
    ret = loadCmdbuf(stream, s->offset() + bin_offset, s->size(), s);
    if (ret != CVI_RC_SUCCESS) {
        return ret;
    }
  }
  return CVI_RC_SUCCESS;
}

CVI_RC CviModel::loadDmabuf(BaseStream *stream, size_t offset, size_t size, const cvi::model::Section *section) {
  if (section->encrypt()) {
    assert(0 && "TODO encrypt");
  }
  CVI_RT_MEM buf = cviMemAlloc(_ctx, size, CVI_ALLOC_DMABUF, _model_name.c_str());
  if (!buf) {
    TPU_LOG_ERROR("alloc memory for dmabuf failed, size:%zu\n", size);
    return CVI_RC_NOMEM;
  }
  stream->read(CVI_RT_MemGetVAddr(buf), offset, size);
  size_t length = size;
  if (section->compress() && section->decompressed_size() > 0) {
#ifdef ENABLE_COMPRESS_CMDBUF
    auto tmp_buf = cviMemAlloc(_ctx, section->decompressed_size(), CVI_ALLOC_DMABUF, _model_name.c_str());
    if (!tmp_buf) {
        TPU_LOG_ERROR("alloc memory for decompressed dmabuf failed, size:%zu\n", size);
        cviMemFree(_ctx, buf);
        return false;
    }
    size_t rc = LZ4_decompress_safe(reinterpret_cast<char *>(CVI_RT_MemGetVAddr(buf)),
                                    reinterpret_cast<char *>(CVI_RT_MemGetVAddr(tmp_buf)),
                                    size, section->decompressed_size());
    TPU_ASSERT(rc == section->decompressed_size(), "decompress error rc != decompressed size");
    cviMemFree(_ctx, buf);
    buf = tmp_buf;
    length = section->decompressed_size();
#else
    TPU_LOG_ERROR("Compressed dmabuf is not supported! please recompile with ENABLE_COMPRESS_CMDBUF\n");
    return CVI_RC_UNSUPPORT;
#endif
  }

  bool enable_pmu = false;
#ifdef ENABLE_PMU
  const char *pmu_enable_env = std::getenv("TPU_ENABLE_PMU");
  if (pmu_enable_env) {
    if (atoi(pmu_enable_env) > 0) {
      enable_pmu = true;
    }
  }
#endif

  CVI_RT_MEM cmdbuf_mem = nullptr;
  int ret = CVI_RT_LoadDmabuf(_ctx, buf, length, 0, 0, enable_pmu, &cmdbuf_mem);
  if (CVI_RC_SUCCESS != ret) {
    cviMemFree(_ctx, buf);
    return ret;
  }
  if (cmdbuf_mem != buf) {
    cviMemFree(_ctx, buf);
  }

  dmabuf_map.emplace(section->name()->str(), cmdbuf_mem);
  return CVI_RC_SUCCESS;
}

CVI_RC CviModel::loadCmdbuf(BaseStream *stream, size_t offset, size_t size, const cvi::model::Section *section) {
  //assert(size && _weight_mem); // load cmdbuf must behind load weight
  if (0 == size) {
    return CVI_RC_SUCCESS;
  }
  std::vector<uint8_t> cmdbuf(size);
  bool enable_pmu = false;

  stream->read(cmdbuf.data(), offset, size);

#ifdef ENABLE_PMU
  const char *pmu_enable_env = std::getenv("TPU_ENABLE_PMU");
  if (pmu_enable_env) {
    if (atoi(pmu_enable_env) > 0) {
      enable_pmu = true;
    }
  }
#endif

  CVI_RT_MEM cmdbuf_mem;
  CVI_RC ret;
  if (section->encrypt()) {
    uint32_t weight_size = CVI_RT_MemGetSize(_weight_mem);
    ret = CVI_RT_LoadCmdbufTee(_ctx, cmdbuf.data(), size, 0,
                               0, weight_size, &cmdbuf_mem);
  } else {
    if (section->compress() && section->decompressed_size()) {
#ifdef ENABLE_COMPRESS_CMDBUF
      uint8_t *buf = new(std::nothrow) uint8_t[section->decompressed_size()];
      TPU_ASSERT(buf != nullptr, "Allocate decompress buff failed");

      size_t rc = LZ4_decompress_safe(reinterpret_cast<char *>(cmdbuf.data()),
                                      reinterpret_cast<char *>(buf),
                                      size, section->decompressed_size());
      TPU_ASSERT(rc == section->decompressed_size(), "decompress error rc != decompressed size");

      ret = CVI_RT_LoadCmdbuf(
          _ctx, buf, section->decompressed_size(), 0,
          0, enable_pmu, &cmdbuf_mem);
      delete[] buf;
#else
      TPU_LOG_ERROR("Compressed cmdbuf is not supported! please recompile with ENABLE_COMPRESS_CMDBUF\n");
      return CVI_RC_UNSUPPORT;
#endif
    } else {
      // setup base address of neuron & weight gmem
      // then load cmdbuf to gmem
      ret = CVI_RT_LoadCmdbuf(
          _ctx, cmdbuf.data(), size, 0,
          0, enable_pmu, &cmdbuf_mem);
    }
  }
  if (ret == CVI_RC_SUCCESS) {
    dmabuf_map.emplace(section->name()->str(), cmdbuf_mem);
  } else {
    TPU_LOG_WARNING("loadCmdbuf failed\n");
  }
  if (isprotect) {
    mem_protect(CVI_RT_MemGetVAddr(cmdbuf_mem), CVI_RT_MemGetSize(cmdbuf_mem));
  }
  return ret;
}

CVI_RC CviModel::loadWeight(BaseStream *stream, size_t offset, size_t size) {
  /// debug
  if (size == 0) {
    return CVI_RC_SUCCESS;
  }
  size_t alloc_size = size;
  if (isprotect) {
    int pageSize = getpagesize();
    alloc_size = ((size + pageSize -1) / pageSize) * pageSize;
  }
  _weight_mem = cviMemAlloc(_ctx, alloc_size, CVI_ALLOC_WEIGHT, _model_name.c_str());
  if (!_weight_mem) {
    TPU_LOG_ERROR("alloc memory for weight failed, size:%zu\n", size);
    return CVI_RC_NOMEM;
  }
  stream->read(CVI_RT_MemGetVAddr(_weight_mem), offset, size);
  CVI_RT_MemFlush(_ctx, _weight_mem);
  if (isprotect) {
    mem_protect(CVI_RT_MemGetVAddr(_weight_mem), alloc_size);
  }
  return CVI_RC_SUCCESS;
}

void CviModel::createCpuWeightMap() {
  if (!_fb_model->weight_map()) {
    return;
  }

  auto &weights = *_fb_model->weight_map();
  for (auto w : weights) {
    if (w->shape()) {
      auto weight = std::make_shared<Neuron>(_ctx, w, _weight_mem, _model_name.c_str());
      weight_map[w->name()->str()] = weight;
    }
  }
}

CVI_RC CviModel::parse(BaseStream *stream) {
  CVI_RC ret;
  size_t payload_size;
  size_t header_size;
  ret = parseModelHeader(stream, payload_size, header_size);
  if (ret != CVI_RC_SUCCESS) {
    return ret;
  }
  size_t bin_offset = header_size + payload_size;
  _model_body = new uint8_t[payload_size];
  if (!_model_body) {
    TPU_LOG_ERROR("Failed to allocate memory\n");
    return CVI_RC_NOMEM;
  }
  stream->read(_model_body, header_size, payload_size);

  _fb_model = (cvi::model::Model *)cvi::model::GetModel(_model_body);
  ret = showAndCheckVersion();
  if (ret != CVI_RC_SUCCESS) {
    return ret;
  }

  std::stringstream model_name;
  model_name << _fb_model->name()->str() << ":" << _count;
  _model_name = model_name.str();

  ret = extractSections(stream, bin_offset);
  if (ret != CVI_RC_SUCCESS) {
    return ret;
  }

  parseProgramNum();
  createCpuWeightMap();

  return CVI_RC_SUCCESS;
}

CVI_RC CviModel::loadProgram(Program **program,
                             int program_id,
                             bool export_all_tensors,
                             bool skip_preprocess) {
  CVI_RC ret;
  auto &programs = *_fb_model->programs();
  assert(program_id < program_num);
  auto fb_program = programs[program_id];
  auto ptr = new Program(_ctx, _pool, dmabuf_map,
                          _cpu_functions, weight_map,
                          _weight_mem, _model_name.c_str(), _max_shared_mem_size);
  if (!ptr) {
    TPU_LOG_ERROR("Failed to create a Program instance\n");
    return CVI_RC_FAILURE;
  }
  ptr->setOptions(export_all_tensors, skip_preprocess);
  ret = ptr->load(fb_program);
  if (ret != CVI_RC_SUCCESS) {
    TPU_LOG_ERROR("program load failed:%d\n", ret);
    delete ptr;
    *program = nullptr;
    return ret;
  }
  *program = ptr;
  return CVI_RC_SUCCESS;
}

std::string CviModel::getChipType(
    const std::string &modelFile,
    const int8_t *buf, size_t size) {
  BaseStream *stream;
  if (!modelFile.empty()) {
    stream = new FileStream(modelFile);
  } else if (buf) {
    stream = new BufferStream(buf, size);
  } else {
    assert(0);
  }
  if (stream->length() <= sizeof(MODEL_HEADER)) {
    TPU_LOG_ERROR("Error, invalid cvimodel file\n");
    assert(0);
  }
  MODEL_HEADER header;
  stream->read((uint8_t *)&header, 0, sizeof(header));
  delete stream;
  return std::string(header.chip);
}

CVI_RC CviModel::acquire(const int8_t *buf, size_t size) {
  BaseStream *stream = new BufferStream(buf, size);
  CVI_RC ret = this->parse(stream);
  if (ret != CVI_RC_SUCCESS) {
    TPU_LOG_ERROR("failed to parse cvimodel\n");
  }
  delete stream;
  return ret;
}

CVI_RC CviModel::acquire(const std::string &modelFile) {
  BaseStream *stream = new FileStream(modelFile);
  CVI_RC ret = this->parse(stream);
  if (ret != CVI_RC_SUCCESS) {
    TPU_LOG_ERROR("failed to parse cvimodel\n");
  }
  delete stream;
  return ret;
}

/*
fd:The file descriptor
ud_offset:The file header offset defined by the user.
*/
CVI_RC CviModel::acquire(const int fd, const size_t ud_offset) {
  BaseStream *stream = new FdStream(fd, ud_offset);
  CVI_RC ret = this->parse(stream);
  if (ret != CVI_RC_SUCCESS) {
    TPU_LOG_ERROR("failed to parse cvimodel\n");
  }
  delete stream;
  return ret;
}

void CviModel::release() {
  --ref;
  if (ref == 0) {
    delete this;
  }
}

} // namespace runtime
} // namespace cvi
