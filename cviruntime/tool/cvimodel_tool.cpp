#include <stdio.h>
#include <math.h>
#include <chrono>
#include <iomanip>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <runtime/debug.h>
#include <runtime/version.h>
#include <runtime/model.hpp>
#include <cvibuilder/cvimodel_generated.h>
#include <cvibuilder/parameter_generated.h>
#include "argparse.hpp"
#include "assert.h"
#include "md5.hpp"

#ifdef ENABLE_COMPRESS_CMDBUF
#include "lz4.h"
#endif

using FBWeightVector =
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cvi::model::Weight>>>;
using FBTensorVector =
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cvi::model::Tensor>>>;
using FBRoutineVector =
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cvi::model::Routine>>>;
using FBProgramVector =
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cvi::model::Program>>>;
using FBSectionVector =
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cvi::model::Section>>>;
using SectionInfoVector =
    std::vector<std::tuple<cvi::runtime::FileStream *, uint32_t, uint32_t, uint32_t>>;

class Model {
public:
  Model(const std::string &model_file);
  ~Model() {
    if (fb_model_buffer)
      delete[] fb_model_buffer;
  }

  void dump();
  void extract();
  void encrypt(std::string &encrypter, std::string &output);
  void merge(std::vector<std::shared_ptr<Model>> &models, std::string &dst);
  void compress_inst(std::string &output);

  cvi::runtime::MODEL_HEADER header;
  cvi::runtime::FileStream stream;
  size_t binary_offset;
  const cvi::model::Model *fb_model;

private:
  uint8_t *fb_model_buffer;
  flatbuffers::FlatBufferBuilder fbb;
  float _qscale = 0;
  std::string input_quanted_tensor;

  void storeSectionToFile(const cvi::model::Section *section, std::string dst);
  size_t compressSectionToFile(const cvi::model::Section *section, std::string dst);
  std::string calcSectionMD5(const cvi::model::Section *section, int size);
  const char *sectionTypeToStr(cvi::model::SectionType type);
  const char *dtypeToStr(cvi::model::DType type);
  size_t dtypeSize(cvi::model::DType type);
  bool getQscaleFromDequantCpuOp(const cvi::cpu_op::Parameter *param);

  void dumpBaseInfo();
  void dumpSections();
  void dumpWeightMap();
  void dumpPrograms();
  void dumpTensors(const cvi::model::Program *p, bool oldVersion);
  void dumpRoutines(const cvi::model::Program *p);

  void extractSections();
  FBWeightVector cloneWeightMap(std::vector<std::shared_ptr<Model>> &models);
  FBTensorVector cloneTensorMap(const cvi::model::Program *program);
  FBRoutineVector
  cloneRoutines(const cvi::model::Program *program, bool rename, int index,
                std::map<std::string, std::string> *routine_name_map);
  FBProgramVector clonePrograms(
      std::vector<std::shared_ptr<Model>> &models,
      std::vector<std::map<std::string, std::string>> &routine_name_maps);
  FBSectionVector cloneSections(
      std::vector<std::shared_ptr<Model>> &models,
      std::vector<uint8_t> &sections_buf,
      std::vector<std::map<std::string, std::string>> &routine_name_maps);

  FBWeightVector rebuildWeightMap();
  FBProgramVector rebuildPrograms();
  FBSectionVector rebuildSections(std::string dir, std::string model_name,
                                  std::vector<uint8_t> &enc_buf);
  FBSectionVector rebuildSections(std::string tmp_dir, std::string model_name,
                                  std::vector<uint8_t> &out_buf,
                                  std::map<std::string, size_t> &cmpr_info);
  int decompress_section(const cvi::model::Section *section,
                         std::vector<char> &out_buf);
  };

Model::Model(const std::string &model_file) : stream(model_file), fbb(0) {
  if (stream.length() <= sizeof(header)) {
    printf("Error, invalid cvimodel file\n");
  }
  stream.read((uint8_t *)&header, 0, sizeof(header));
  size_t header_size;
  /* before version 1.1, heder size is 32 bytes */
  if (header.major == 1 && header.minor == 0)
    header_size = 0x20;
  else
    header_size = sizeof(cvi::runtime::MODEL_HEADER);
  binary_offset = header_size + header.body_size;
  fb_model_buffer = new uint8_t[header.body_size];
  if (!fb_model_buffer) {
    printf("Failed to allocate memory\n");
  }
  stream.read(fb_model_buffer, header_size, header.body_size);
  fb_model = cvi::model::GetModel(fb_model_buffer);
}

void Model::dump() {
  dumpBaseInfo();
  dumpSections();
  dumpWeightMap();
  dumpPrograms();
}

void Model::extract() {
  auto name = fb_model->name()->str();
  for (auto p : *fb_model->programs()) {
    auto neuron_size = p->neuron_size();
    auto private_gmem_size = p->private_gmem();
    auto shared_gmem_size = p->shared_gmem();
    if (neuron_size) {
      std::cout << "neuron_size: " << neuron_size << "\n";
    } else {
      std::cout << "private_mem_size: " << private_gmem_size << "\n";
      std::cout << "shared_gmem_size: " << shared_gmem_size << "\n";
    }
    // dump cmdbuf
    for (auto r : *p->routines()) {
      if (r->type() != cvi::model::RoutineType_TPU)
        continue;

      auto getTensorByName = [&](std::string name) {
        for (const auto &t : *p->tensor_map()) {
          if (t->name()->str() == name) {
            return t;
          }
        }
        assert(0);
      };


      std::string buf_name;
      std::string buf_type_name;
      if (r->tpu_routine()->cmdbuf_section()) {
        buf_name = r->tpu_routine()->cmdbuf_section()->str();
        buf_type_name = "cmdbuf";
      } else if (r->tpu_routine()->dmabuf_section()) {
        buf_name = r->tpu_routine()->dmabuf_section()->str();
        buf_type_name = "dmabuf";
      } else {
        assert(0 && "model has not cmdbuf and dmabuf");
      }

      printf("routine #%s\n", buf_name.c_str());
      printf("  %-6s %-4s %-4s %-4s %-4s %-5s %-7s %s\n", " ", "n", "c", "h", "w",
             "dtype", "offset", "name");
      for (auto name : *r->in_tensors()) {
        auto tensor = getTensorByName(name->str());
        auto &shape = *tensor->shape()->dim();
        printf("  %-6s %-4d %-4d %-4d %-4d %-5s %-7d %s\n", "[IN ]", (int)shape[0],
               (int)shape[1], (int)shape[2], (int)shape[3], dtypeToStr(tensor->dtype()),
               (int)tensor->offset(), name->c_str());
      }
      for (auto name : *r->out_tensors()) {
        auto tensor = getTensorByName(name->str());
        auto &shape = *tensor->shape()->dim();
        printf("  %-6s %-4d %-4d %-4d %-4d %-5s %-7d %s\n", "[OUT]", (int)shape[0],
               (int)shape[1], (int)shape[2], (int)shape[3], dtypeToStr(tensor->dtype()),
               (int)tensor->offset(), name->c_str());
      }
      for (auto s : *fb_model->sections()) {
        if (s->name()->str() == buf_name) {
          std::string dst = name + "_program_" + std::to_string(p->batch_num()) +
                            "_" + buf_type_name + "_" + s->name()->str() + ".bin";
          storeSectionToFile(s, dst);
          break;
        }
      }
    }
  }
  // dump weight
  for (auto s : *fb_model->sections()) {
    if (s->type() != cvi::model::SectionType_WEIGHT)
      continue;
    storeSectionToFile(s, name + "_weight.bin");
  }
}

int Model::decompress_section(const cvi::model::Section *section, std::vector<char>& out_buf) {
  if (!section->compress()) {
    return 0;
  }
#ifdef ENABLE_COMPRESS_CMDBUF
  auto size = section->size();
  auto offset = section->offset();

  char *in_buf = new (std::nothrow) char[size];
  if (!in_buf) {
    printf("Alloc buff for decompress failed! buff size:%d\n", size);
    return -1;
  }
  out_buf.resize(section->decompressed_size());

  stream.read(reinterpret_cast<uint8_t*>(in_buf), binary_offset + offset, size);

  size_t rc = LZ4_decompress_safe(in_buf, out_buf.data(), size, section->decompressed_size());
  delete[] in_buf;
  if (rc != section->decompressed_size()) {
    printf("Decompress section failed! section name:%s\n", section->name()->c_str());
    return -1;
  }
  return 0;
#else
  printf("Compressed section is not supported! please recompile with ENABLE_COMPRESS_CMDBUF\n");
  return -1;
#endif
}

void Model::storeSectionToFile(const cvi::model::Section *section, std::string dst) {
  auto offset = section->offset();
  auto size = section->size();
  std::ofstream of(dst, std::ofstream::out | std::ofstream::binary |
                            std::ofstream::trunc);
  if (section->compress()) {
    std::vector<char> out_buf;
    if (0 != decompress_section(section, out_buf)) {
      printf("store section failed\n");
      return;
    }
    of.write(out_buf.data(), out_buf.size());
  } else {
    uint8_t *buf = new uint8_t[1024];
    do {
      auto len = size > 1024 ? 1024 : size;
      stream.read(buf, binary_offset + offset, len);
      of.write((const char *)buf, len);
      offset += len;
      size -= len;
    } while (size);
    of.close();
    delete[] buf;
  }
  printf("store section to %s\n", dst.c_str());
}

std::string Model::calcSectionMD5(const cvi::model::Section *section, int size) {
  auto offset = section->offset();
  MD5 md5;
  if (section->compress()) {
    std::vector<char> out_buf;
    if (0 != decompress_section(section, out_buf)) {
      printf("store section failed\n");
      return std::string();
    }
    md5.update(reinterpret_cast<uint8_t*>(out_buf.data()), out_buf.size());
  } else {
    uint8_t *buf = new uint8_t[1024];
    do {
      auto len = size > 1024 ? 1024 : size;
      stream.read(buf, binary_offset + offset, len);
      md5.update(buf, len);
      offset += len;
      size -= len;
    } while (size);
    delete[] buf;
  }
  return md5.finalize().hexdigest();
}

void Model::dumpBaseInfo() {
  if (fb_model->mlir_version()) {
    printf("Mlir Version: %s\n", fb_model->mlir_version()->c_str());
  }
  auto version = fb_model->version();
  printf("Cvimodel Version: %d.%d.%d\n", (int)version->major_(), (int)version->minor_(),
         (int)version->sub_minor());
  printf("%s Build at %s\n", fb_model->name()->c_str(), fb_model->build_time()->c_str());
  if (fb_model->target()) {
    printf("For %s chip ONLY\n", fb_model->target()->c_str());
  }

  // dump peak memory usage, summary static size(weight/cmdbuf) and runtime (private_gmem_size+shared_gmem_size+io_mem)
  size_t total_size = 0;
  auto &sections = *fb_model->sections();
  // static
  for (auto s : sections) {
    total_size += s->size();
  }

  // runtime
  auto &programs = *fb_model->programs();
  size_t share_size = 0;
  size_t io_size = 0;
  for (auto p : programs) {
    total_size += p->neuron_size();
    total_size += p->private_gmem();
    if (share_size < p->shared_gmem()) {
      share_size = p->shared_gmem();
    }
    auto &tensor_map = *p->tensor_map();
    for (auto t : tensor_map) {
      auto gaddr = (int64_t)t->offset();
      if (gaddr != -1) {
        auto memTypeIndx = (gaddr >> 40) & 0x07;
        bool oldVersion = p->neuron_size() > 0;
        if (memTypeIndx > 1 || oldVersion) {
          if (memTypeIndx > 2) {
            // io_mem
            auto &shape = *t->shape()->dim();
            size_t type_size = dtypeSize(t->dtype());
            size_t tensor_size = shape[0] * shape[1] * shape[2] * shape[3] * type_size;
            io_size += tensor_size;
          }
        }
      }
    }
  }
  total_size += share_size;
  total_size += io_size;
  printf("CviModel Need ION Memory Size: (%.2f MB)\n", total_size / (float)(1024*1024));
}

void Model::dumpSections() {
  printf("\nSections:\n");
  printf("%-3s  %-10s%-25s%-12s%-12s%-12s%-12s%-s\n", "ID", "TYPE", "NAME", "SIZE", "OFFSET",
         "ENCRYPT", "COMPRESS", "MD5");
  auto &sections = *fb_model->sections();
  int i = 0;
  for (auto s : sections) {
    auto type = sectionTypeToStr(s->type());
    auto name = s->name()->c_str();
    auto size = s->size();
    auto offset = s->offset();
    auto md5 = calcSectionMD5(s, s->size());
    auto encrypt = s->encrypt();
    auto compress = s->compress();
    printf("%03d  %-10s%-25s%-12d%-12d%-12s%-12s%-s\n", i++, type, name, size, offset,
           encrypt ? "True" : "False", compress ? "True" :"False", md5.c_str());
  }
}

void Model::dumpWeightMap() {
  printf("\nWeightMap:\n");
  printf("%-3s  %-10s%-10s%-8s%-4s %-4s %-4s %-4s %-s\n", "ID", "OFFSET", "SIZE", "TYPE",
         "N", "C", "H", "W", "NAME");

  auto &weights = *fb_model->weight_map();
  int i = 0;
  for (auto w : weights) {
    auto &shape = *w->shape()->dim();
    printf("%03d  %-10d%-10d%-8s%-4d %-4d %-4d %-4d %-s\n", i++, (int)w->offset(),
           w->size(), dtypeToStr(w->type()), (int)shape[0], (int)shape[1], (int)shape[2],
           (int)shape[3], w->name()->c_str());
  }
}

void Model::dumpPrograms() {
  auto &programs = *fb_model->programs();
  int idx = 0;
  for (auto p : programs) {
    auto batch_num = p->batch_num();
    auto neuron_size = p->neuron_size();
    auto private_gmem_size = p->private_gmem();
    auto shared_gmem_size = p->shared_gmem();
    auto &input_tensors = *p->input_tensors();
    auto &output_tensors = *p->output_tensors();
    printf("\nProgram #%d\n", idx++);
    printf("    %-12s: %d\n", "batch_num", batch_num);
    if (neuron_size) {
      printf("    %-12s: %d\n", "neuron_size", neuron_size);
    } else {
      printf("    %-12s: %d\n", "private_gmem_size", private_gmem_size);
      printf("    %-12s: %d\n", "shared_gmem_size", shared_gmem_size);
    }
    printf("    %-12s: ", "inputs");
    for (int i = 0; i < (int)input_tensors.size(); i++) {
      if (i != 0)
        printf(",");
      printf("%s", input_tensors[i]->c_str());
    }
    printf("\n    %-12s: ", "outputs");
    for (int i = 0; i < (int)output_tensors.size(); i++) {
      if (i != 0)
        printf(",");
      printf("%s", output_tensors[i]->c_str());
    }
    printf("\n    %-12s:\n", "routines");
    dumpRoutines(p);
    printf("\n    %-12s:\n", "tensor_map");
    // The cvimodel is old version(blow 1.1.0)
    // if neuson size is greater than 0,
    dumpTensors(p, neuron_size > 0);
  }
}

void Model::dumpTensors(const cvi::model::Program *p, bool oldVersion) {
  printf("        ");
  printf("%-3s  %-12s%-6s%-4s %-4s %-4s %-4s %-10s %-7s %-s\n", "ID", "OFFSET", "TYPE",
         "N", "C", "H", "W", "QSCALE", "MEM", "NAME");
  auto &tensors = *p->tensor_map();
  int i = 0;
  for (auto t : tensors) {
    auto &shape = *t->shape()->dim();
    std::string memType = "   -";
    auto gaddr = (int64_t)t->offset();
    if (gaddr != -1) {
      auto memTypeIndx = (gaddr >> 40) & 0x07;
      if (memTypeIndx > 1 || oldVersion) {
        if (memTypeIndx > 2) {
          memType = "io_mem";
        } else {
          memType = "private";
        }
      } else {
        memType = "shared";
      }
    }
    float qscale = t->quant() ? t->quant()->qscale() : 0;
    if (t->name()->str() == input_quanted_tensor) {
      qscale = _qscale;
    }
    printf("        ");
    if (qscale <= 0.000001 || qscale > 400.0f) {
      printf("%03d  %-12d%-6s%-4d %-4d %-4d %-4d %-10s %-7s %-s\n", i++, (int)t->offset(),
             dtypeToStr(t->dtype()), (int)shape[0], (int)shape[1], (int)shape[2],
             (int)shape[3], "-", memType.c_str(), t->name()->c_str());
    } else {
      printf("%03d  %-12d%-6s%-4d %-4d %-4d %-4d %-10f %-7s %-s\n", i++, (int)t->offset(),
             dtypeToStr(t->dtype()), (int)shape[0], (int)shape[1], (int)shape[2],
             (int)shape[3], qscale, memType.c_str(), t->name()->c_str());
    }
  }
}

bool Model::getQscaleFromDequantCpuOp(const cvi::cpu_op::Parameter *param) {
  std::string from;
  std::string to;
  float threshold = 0;
  auto &attributes = *param->attributes();
  for (auto attr : attributes) {
    if (attr->float_attr()) {
      auto _float = attr->float_attr();
      if (_float->key()->str() == "threshold") {
        threshold = _float->value();
      }
    } else if (attr->str_attr()) {
      auto _str = attr->str_attr();
      if (_str->key()->str() == "from") {
        from = _str->value()->str();
      } else if (_str->key()->str() == "to") {
        to = _str->value()->str();
      }
    }
  }
  if (threshold != 0 && from == "NONE" && to == "INT8") {
    _qscale = 128.0 / threshold;
    return true;
  }
  return false;
}

void Model::dumpRoutines(const cvi::model::Program *p) {
  auto &routines = *p->routines();
  int i = 0;
  for (auto r : routines) {
    bool tpu = r->type() == cvi::model::RoutineType_TPU;
    printf("     #%02d  %s\n", i++, tpu ? "tpu" : "cpu");
    printf("        %-8s: ", "inputs");
    int j = 0;
    for (auto name : *r->in_tensors()) {
      if (j++ != 0)
        printf(",");
      printf("%s", name->c_str());
    }
    printf("\n        %-8s: ", "outputs");
    j = 0;
    for (auto name : *r->out_tensors()) {
      if (j++ != 0)
        printf(",");
      printf("%s", name->c_str());
    }
    if (tpu) {
      std::string buf_name;
      if (r->tpu_routine()->cmdbuf_section()) {
        buf_name = r->tpu_routine()->cmdbuf_section()->str();
      } else if (r->tpu_routine()->dmabuf_section()) {
        buf_name = r->tpu_routine()->dmabuf_section()->str();
      } else {
        assert(0 && "model has not cmdbuf and dmabuf");
      }
      printf("\n        %-8s: %s\n", "section",
             buf_name.c_str());
    } else {
      if (r->cpu_routine()->function_section()->str() == "quant" && _qscale == 0) {
        auto param = cvi::cpu_op::GetParameter(r->cpu_routine()->function_args()->data());
        if (getQscaleFromDequantCpuOp(param)) {
          input_quanted_tensor = (*r->out_tensors())[0]->str();
        }
      }
      printf("\n        %-8s: %s\n", "function",
             r->cpu_routine()->function_section()->c_str());
    }
  }
}

const char *Model::sectionTypeToStr(cvi::model::SectionType type) {
  switch (type) {
    case cvi::model::SectionType_WEIGHT:
      return "weight";
    case cvi::model::SectionType_CMDBUF:
      return "cmdbuf";
    case cvi::model::SectionType_DMABUF:
      return "dmabuf";
    case cvi::model::SectionType_FUNC_X86:
      return "x86_64";
    case cvi::model::SectionType_FUNC_AARCH64:
      return "aarch64";
    default:
      printf("unknown section type\n");
  }
  return "";
}

const char *Model::dtypeToStr(cvi::model::DType type) {
  switch (type) {
    case cvi::model::DType_FP32:
      return "fp32";
    case cvi::model::DType_INT32:
      return "int32";
    case cvi::model::DType_UINT32:
      return "uint32";
    case cvi::model::DType_BF16:
      return "bf16";
    case cvi::model::DType_INT16:
      return "int16";
    case cvi::model::DType_UINT16:
      return "uint16";
    case cvi::model::DType_INT8:
      return "int8";
    case cvi::model::DType_UINT8:
      return "uint8";
    default:
      printf("unknown dtype\n");
  }
  return "";
}

size_t Model::dtypeSize(cvi::model::DType type) {
  switch (type) {
    case cvi::model::DType_FP32:
      return 4;
    case cvi::model::DType_INT32:
      return 4;
    case cvi::model::DType_UINT32:
      return 4;
    case cvi::model::DType_BF16:
      return 2;
    case cvi::model::DType_INT16:
      return 2;
    case cvi::model::DType_UINT16:
      return 2;
    case cvi::model::DType_INT8:
      return 1;
    case cvi::model::DType_UINT8:
      return 1;
    default:
      printf("unknown dtype\n");
  }
  return 0;
}

static std::string getStrOfCurrentTime() {
  std::stringstream ssTime;
  auto clockNow = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(clockNow);
  ssTime << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
  return ssTime.str();
}

FBWeightVector Model::cloneWeightMap(std::vector<std::shared_ptr<Model>> &models) {
  std::vector<flatbuffers::Offset<cvi::model::Weight>> tensor_vec;
  std::vector<std::tuple<std::string, int64_t, uint32_t>> weight_tensors;
  bool redundant = false;
  for (auto &model : models) {
    for (auto w : *model->fb_model->weight_map()) {
      redundant = false;
      auto name = w->name()->c_str();
      for (auto t : weight_tensors) {
        if (std::get<0>(t) == name && std::get<1>(t) == w->offset() && std::get<2>(t) == w->size()) {
          redundant = true;
          break;
        }
      }
      if (redundant) continue;
      std::vector<int64_t> dim;
      for (auto s : *w->shape()->dim()) {
        dim.push_back(s);
      }
      auto shape = cvi::model::CreateShapeDirect(fbb, &dim);
      auto weight = cvi::model::CreateWeightDirect(fbb, name, w->offset(), w->size(), shape,
                                                  w->type());
      tensor_vec.push_back(weight);
      weight_tensors.push_back(std::make_tuple(name, w->offset(), w->size()));
    }
  }
  return fbb.CreateVector(tensor_vec);
}

FBTensorVector Model::cloneTensorMap(const cvi::model::Program *program) {
  std::vector<flatbuffers::Offset<cvi::model::Tensor>> tensor_vec;
  for (auto t : *program->tensor_map()) {
    auto name = t->name()->c_str();
    std::vector<int64_t> dim;
    for (auto s : *t->shape()->dim()) {
      dim.push_back(s);
    }
    auto shape = cvi::model::CreateShapeDirect(fbb, &dim);
    auto tensor = cvi::model::CreateTensorDirect(fbb, t->tensor_id(), name, t->offset(), t->dtype(),
                                       shape, 0, 0, t->overwrote());
    if (t->quant()) {
      auto quant = cvi::model::CreateQuantInfo(fbb, t->quant()->type(), 0, 0,
                                             t->quant()->zero_point(),
                                             t->quant()->qscale());
      std::vector<float> scale;
      if (t->scale()) {
        for (int i = 0; i < (int)t->scale()->size(); ++i) {
          scale.push_back(t->scale()->Get(i));
        }
      }

      std::vector<float> mean;
      if (t->mean()) {
        for (int i = 0; i < (int)t->mean()->size(); ++i) {
          mean.push_back(t->mean()->Get(i));
        }
      }

      std::string pixel_format;
      if (t->pixel_format()) {
        pixel_format = t->pixel_format()->str();
      }

      tensor = cvi::model::CreateTensorDirect(fbb, t->tensor_id(), name, t->offset(), t->dtype(),
                                      shape, 0, quant, t->overwrote(),
                                      scale.size() > 0 ? &scale : nullptr,
                                      mean.size() > 0 ? &mean : nullptr,
                                      pixel_format.length() > 0 ? pixel_format.c_str() : nullptr,
                                      t->aligned(), t->size());
    }
    tensor_vec.push_back(tensor);
  }
  return fbb.CreateVector(tensor_vec);
}

FBRoutineVector
Model::cloneRoutines(const cvi::model::Program *program, bool rename, int index,
                     std::map<std::string, std::string> *routine_name_map) {
  std::vector<flatbuffers::Offset<cvi::model::Routine>> routines;
  for (auto r : *program->routines()) {
    std::vector<flatbuffers::Offset<flatbuffers::String>> fbStrVec;
    for (auto name : *r->in_tensors()) {
      fbStrVec.push_back(fbb.CreateString(name));
    }
    auto inputs = fbb.CreateVector(fbStrVec);
    fbStrVec.clear();
    for (auto name : *r->out_tensors()) {
      fbStrVec.push_back(fbb.CreateString(name));
    }
    auto outputs = fbb.CreateVector(fbStrVec);
    if (r->type() == cvi::model::RoutineType_TPU) {
      flatbuffers::Offset<cvi::model::TpuRoutine> tpuRoutine;
      if (rename) {
        std::stringstream new_name;
        if (r->tpu_routine()->cmdbuf_section()) {
          new_name << r->tpu_routine()->cmdbuf_section()->c_str() << "_" << index;
          tpuRoutine = cvi::model::CreateTpuRoutineDirect(
              fbb, new_name.str().c_str(), nullptr);
          routine_name_map->emplace(r->tpu_routine()->cmdbuf_section()->c_str(), new_name.str());
        } else {
          new_name << r->tpu_routine()->dmabuf_section()->c_str() << "_" << index;
          tpuRoutine = cvi::model::CreateTpuRoutineDirect(
              fbb, nullptr, new_name.str().c_str());
          routine_name_map->emplace(r->tpu_routine()->dmabuf_section()->c_str(), new_name.str());
        }
      } else {
        const char *cmdbuf = r->tpu_routine()->cmdbuf_section()
                                 ? r->tpu_routine()->cmdbuf_section()->c_str()
                                 : nullptr;
        const char *dmabuf = r->tpu_routine()->dmabuf_section()
                                 ? r->tpu_routine()->dmabuf_section()->c_str()
                                 : nullptr;
        tpuRoutine =
            cvi::model::CreateTpuRoutineDirect(fbb, cmdbuf, dmabuf);
      }
      auto routine = cvi::model::CreateRoutine(fbb, cvi::model::RoutineType_TPU,
                                               inputs, outputs, tpuRoutine, 0);
      routines.push_back(routine);
    } else {
      std::vector<uint8_t> args;
      for (auto byte : *r->cpu_routine()->function_args()) {
        args.push_back(byte);
      }
      auto cpuRoutine = cvi::model::CreateCpuRoutineDirect(
          fbb, r->cpu_routine()->function_section()->c_str(), &args);
      auto routine =
          cvi::model::CreateRoutine(fbb, r->type(), inputs, outputs, 0, cpuRoutine);
      routines.push_back(routine);
    }
  }
  return fbb.CreateVector(routines);
}

FBProgramVector Model::clonePrograms(
    std::vector<std::shared_ptr<Model>> &models,
    std::vector<std::map<std::string, std::string>> &routine_name_maps) {
  std::vector<flatbuffers::Offset<cvi::model::Program>> programs;
  routine_name_maps.clear();
  for (uint32_t i = 0; i < models.size(); ++i) {
    for (auto p : *models[i]->fb_model->programs()) {
      auto tensor_map = cloneTensorMap(p);
      std::vector<flatbuffers::Offset<flatbuffers::String>> fbStrVec;
      for (auto name : *p->input_tensors()) {
        fbStrVec.push_back(fbb.CreateString(name));
      }
      auto inputs = fbb.CreateVector(fbStrVec);
      fbStrVec.clear();
      for (auto name : *p->output_tensors()) {
        fbStrVec.push_back(fbb.CreateString(name));
      }
      auto outputs = fbb.CreateVector(fbStrVec);
      std::map<std::string, std::string> routine_name_map;
      auto routines = cloneRoutines(p, true, i, &routine_name_map);
      routine_name_maps.emplace_back(std::move(routine_name_map));
      auto program = cvi::model::CreateProgram(fbb, p->batch_num(), p->neuron_size(),
                                               inputs, outputs, tensor_map, routines,
                                               p->shared_gmem(), p->private_gmem());
      programs.push_back(program);
    }
  }
  return fbb.CreateVector(programs);
}

typedef struct {
  int id;
  const cvi::model::Section* section;
  uint32_t size;
} weight_section_t;

FBSectionVector Model::cloneSections(std::vector<std::shared_ptr<Model>> &models,
                                     std::vector<uint8_t> &sections_buf,
                                     std::vector<std::map<std::string, std::string> > &routine_name_maps) {
  uint32_t offset = 0;
  std::string weight_md5 = "";
  std::vector<flatbuffers::Offset<cvi::model::Section>> section_vec;
  std::vector<weight_section_t> weight_sections;
  assert(models.size() == routine_name_maps.size());

  uint8_t bit_buf_type = 0;
  // first select candiate weight section
  for (int i = 0; i < (int)models.size(); ++i) {
    for (auto s : *models[i]->fb_model->sections()) {
      if (s->type() == cvi::model::SectionType_WEIGHT) {
        weight_section_t ws={0};
        ws.id = i;
        ws.section = s;
        ws.size = s->size();
        weight_sections.emplace_back(ws);
      } else if (s->type() == cvi::model::SectionType_CMDBUF) {
        bit_buf_type |= 0x01;
      } else if (s->type() == cvi::model::SectionType_DMABUF) {
        bit_buf_type |= 0x10;
      }
    }
  }

  if (bit_buf_type == 0x11) {
    printf("WARN: models can't include both dmabuf and cmdbuf!\n");
    exit(1);
  }

  std::sort(weight_sections.begin(), weight_sections.end(),
            [](weight_section_t &s1, weight_section_t &s2) {
              return s1.size < s2.size;
            });

  std::vector<int> weight_compare_size;
  std::vector<int> weight_index;
  for (auto pair : weight_sections) {
    weight_index.push_back(pair.id);
    weight_compare_size.push_back(pair.section->size());
  }

  for (int i = weight_compare_size.size() - 1; i > 0; --i) {
    weight_compare_size[i] = weight_compare_size[i-1];
  }

  int candidate_index = weight_index[weight_index.size()-1];

  for (int i = 0; i < (int)weight_sections.size(); ++i) {
    auto &ws = weight_sections[i];
    auto model = models[ws.id];
    auto section = ws.section;
    auto md5 = model->calcSectionMD5(section, weight_compare_size[i]);
    if (weight_md5.empty()) {
      weight_md5 = md5;
    } else {
      if (weight_md5 != md5) {
        printf("WARN: weight binary of cvimodels should be same, model index (%d) vs (%d)\n",
         weight_index[i-1], weight_index[i]);
        exit(1);
      } else {
        weight_md5 = model->calcSectionMD5(section, section->size());
      }
    }
  }

  printf("cvimodels weight compare pass!\n");

  int model_index = 0;
  for (uint32_t i = 0; i < models.size(); ++i) {
    for (auto s : *models[i]->fb_model->sections()) {
      auto md5 = models[i]->calcSectionMD5(s, s->size());
      printf("section type: %d, name: %s, size: %d, offset: %d, compress:%s "
             "md5:%s\n",
             (int)s->type(), s->name()->c_str(), s->size(), s->offset(),
             s->compress() ? "True" : "False", md5.c_str());
      // check if weight binary of all cvimodels are same.
      if (s->type() == cvi::model::SectionType_WEIGHT) {
        if (model_index != candidate_index) {
          continue;
        }
      }

      std::string section_name;
      if (s->type() == cvi::model::SectionType_CMDBUF || s->type() == cvi::model::SectionType_DMABUF) {
        section_name = routine_name_maps[i][s->name()->c_str()];
        assert(!section_name.empty());
      } else {
        section_name = s->name()->c_str();
      }

      printf("add section, name:%s type:%d, md5:%s\n", section_name.c_str(), (int)s->type(), md5.c_str());
      auto section = cvi::model::CreateSectionDirect(fbb, s->type(), section_name.c_str(),
                                                     s->size(), offset, s->encrypt(),
                                                     s->compress(), s->decompressed_size());
      section_vec.push_back(section);
      std::vector<uint8_t> buf(s->size());
      models[i]->stream.read(buf.data(), models[i]->binary_offset + s->offset(), s->size());
      sections_buf.insert(sections_buf.end(), buf.begin(), buf.end());
      offset += s->size();
    }
    model_index++;
  }
  return fbb.CreateVector(section_vec);
}

void Model::merge(std::vector<std::shared_ptr<Model>> &models, std::string &dst) {
  cvi::model::Version modelVersion =
      cvi::model::Version(cvi::model::MajorVersion_value, cvi::model::MinorVersion_value,
                          cvi::model::SubMinorVersion_value);
  std::vector<std::map<std::string, std::string> > routine_name_maps;
  auto modelName = fbb.CreateString(fb_model->name());
  auto modelBuildTime = fbb.CreateString(getStrOfCurrentTime());
  auto modelMlirVersion = fb_model->mlir_version() ?
                          fbb.CreateString(fb_model->mlir_version()):
                          fbb.CreateString("unknown");
  auto fbTarget = fbb.CreateString(fb_model->target());
  auto modelWeight = cloneWeightMap(models);
  auto modelPrograms = clonePrograms(models, routine_name_maps);
  std::vector<uint8_t> sections_buf;
  auto modelSections = cloneSections(models, sections_buf, routine_name_maps);
  auto newModel =
      cvi::model::CreateModel(fbb, &modelVersion, modelName, modelBuildTime,  0, 0,
                              modelWeight, modelPrograms, modelSections,
                              fbTarget, modelMlirVersion);
  fbb.Finish(newModel);

  cvi::runtime::MODEL_HEADER modelHeader;
  std::string magic = u8"CviModel";
  std::string pad = u8"AA";
  memcpy(modelHeader.magic, magic.c_str(), sizeof(modelHeader.magic));
  memcpy(modelHeader.padding, pad.c_str(), sizeof(modelHeader.padding));
  memset(modelHeader.chip, 0, 16);
  memcpy(modelHeader.chip, this->header.chip, sizeof(modelHeader.chip));
  modelHeader.body_size = fbb.GetSize();
  modelHeader.major = cvi::model::MajorVersion_value;
  modelHeader.minor = cvi::model::MinorVersion_value;

  std::ofstream of(dst,
                   std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
  of.write((const char *)&modelHeader, sizeof(modelHeader));
  of.write((const char *)fbb.GetBufferPointer(), fbb.GetSize());
  of.write((const char *)sections_buf.data(), sections_buf.size());
  of.close();
  printf("store cvimodel to %s\n", dst.c_str());
}

FBWeightVector Model::rebuildWeightMap() {
  std::vector<flatbuffers::Offset<cvi::model::Weight>> tensor_vec;
  for (auto w : *fb_model->weight_map()) {
    auto name = w->name()->c_str();
    std::vector<int64_t> dim;
    for (auto s : *w->shape()->dim()) {
      dim.push_back(s);
    }
    auto shape = cvi::model::CreateShapeDirect(fbb, &dim);
    auto weight = cvi::model::CreateWeightDirect(fbb, name, w->offset(), w->size(), shape,
                                                w->type());
    tensor_vec.push_back(weight);
  }
  return fbb.CreateVector(tensor_vec);
}

FBProgramVector Model::rebuildPrograms() {
  std::vector<flatbuffers::Offset<cvi::model::Program>> programs;
  for (auto p : *fb_model->programs()) {
    auto tensor_map = cloneTensorMap(p);
    std::vector<flatbuffers::Offset<flatbuffers::String>> fbStrVec;
    for (auto name : *p->input_tensors()) {
      fbStrVec.push_back(fbb.CreateString(name));
    }
    auto inputs = fbb.CreateVector(fbStrVec);
    fbStrVec.clear();
    for (auto name : *p->output_tensors()) {
      fbStrVec.push_back(fbb.CreateString(name));
    }
    auto outputs = fbb.CreateVector(fbStrVec);
    auto routines = cloneRoutines(p, false, 0, nullptr);
    auto program = cvi::model::CreateProgram(fbb, p->batch_num(), p->neuron_size(),
                                             inputs, outputs, tensor_map, routines,
                                             p->shared_gmem(), p->private_gmem());
    programs.push_back(program);
  }
  return fbb.CreateVector(programs);
}

FBSectionVector Model::rebuildSections(std::string tmp_dir, std::string model_name,
                                       std::vector<uint8_t> &enc_buf) {
  uint32_t offset = 0;
  uint32_t enc_offset = 0;
  std::vector<flatbuffers::Offset<cvi::model::Section>> section_vec;
  for (auto s : *fb_model->sections()) {
    if (s->type() == cvi::model::SectionType_WEIGHT ||
        s->type() == cvi::model::SectionType_CMDBUF ||
        s->type() == cvi::model::SectionType_DMABUF) {
      std::string enc_filename =
          tmp_dir + "/" + model_name + "_" + s->name()->c_str() + ".bin.enc";
      std::ifstream ifs(enc_filename, std::ios::in | std::ios::binary);
      ifs.seekg(0, ifs.end);
      size_t size = ifs.tellg();
      ifs.seekg(0, ifs.beg);
      printf("filename: %s, filesize: %d\n", enc_filename.c_str(), (int)size);
      char *buf = new char[size];
      if (buf == nullptr) {
        printf("alloc buf fail\n");
        exit(-1);
      }
      ifs.read(buf, size);
      enc_buf.insert(enc_buf.end(), buf, buf + size);
      delete[] buf;
      ifs.close();

      auto section = cvi::model::CreateSectionDirect(fbb, s->type(), s->name()->c_str(),
                                                     size, enc_offset, true);
      section_vec.push_back(section);
      enc_offset += size;
    } else {
      uint8_t *buf = new uint8_t[s->size()];
      if (buf == nullptr) {
        printf("alloc buf fail\n");
        exit(-1);
      }
      stream.read(buf, binary_offset + offset, s->size());
      enc_buf.insert(enc_buf.end(), buf, buf + s->size());
      delete[] buf;

      auto section = cvi::model::CreateSectionDirect(fbb, s->type(), s->name()->c_str(),
                                                     s->size(), enc_offset, false);
      section_vec.push_back(section);
      enc_offset += s->size();
    }
    offset += s->size();
  }

  return fbb.CreateVector(section_vec);
}

void Model::encrypt(std::string &encrypter, std::string &output) {
  // create tmp dir to keep extracted cmdbuf and weight
  std::string tmp_dir = ".tmp_dir";
  struct stat file_stat;
  if (stat(tmp_dir.c_str(), &file_stat) != 0) {
    mkdir(tmp_dir.c_str(), 0777);
  } else {
    printf("create tmp directory fail: file exist!\n");
    return;
  }

  auto model_name = fb_model->name()->str();
  std::vector<std::string> cmdbuf_names;
  std::vector<std::string> dmabuf_names;
  std::string weight_name;

  for (auto p : *fb_model->programs()) {
    for (auto r : *p->routines()) {
      if (r->type() != cvi::model::RoutineType_TPU)
        continue;

      std::vector<std::string> *names = nullptr;
      std::string name;
      if (r->tpu_routine()->cmdbuf_section()) {
        names = &cmdbuf_names;
        name = r->tpu_routine()->cmdbuf_section()->str();
      } else if (r->tpu_routine()->dmabuf_section()) {
        assert(0 && "unsupport encrypt dmabuf");
        names = &dmabuf_names;
        name = r->tpu_routine()->dmabuf_section()->str();
      } else {
        assert(0);
      }

      printf("routine #%s\n", name.c_str());

      for (auto s : *fb_model->sections()) {
        if (s->name()->str() == name) {
          std::string dst = model_name + "_" + s->name()->str() + ".bin";
          names->push_back(dst);
          storeSectionToFile(s, std::string(tmp_dir + "/" + dst));
          break;
        }
      }
    }
  }

  for (auto s : *fb_model->sections()) {
    if (s->type() != cvi::model::SectionType_WEIGHT)
      continue;
    weight_name = model_name + "_" + s->name()->str() + ".bin";
    storeSectionToFile(s, std::string(tmp_dir + "/" + weight_name));
  }

  for (auto cmdbuf : cmdbuf_names) {
    int pid = fork();
    if (pid == 0) {
      printf("encrypt cmdbuf: %s in pid: %d\n", cmdbuf.c_str(), getpid());
      execl(encrypter.c_str(), "cvi_crypt", "encrypt_sign_aimodel",
            std::string(tmp_dir + "/" + cmdbuf).c_str(),
            std::string(tmp_dir + "/" + weight_name).c_str(), nullptr);
    }
  }
  for (auto dmabuf : dmabuf_names) {
    int pid = fork();
    if (pid == 0) {
      printf("encrypt dmabuf: %s in pid: %d\n", dmabuf.c_str(), getpid());
      execl(encrypter.c_str(), "cvi_crypt", "encrypt_sign_aimodel",
            std::string(tmp_dir + "/" + dmabuf).c_str(),
            std::string(tmp_dir + "/" + weight_name).c_str(), nullptr);
    }
  }

  while (wait(nullptr) > 0)
    ;

  printf("encrypt cmdbuf and weight success!\n");

  std::vector<uint8_t> enc_buf;
  cvi::model::Version modelVersion =
      cvi::model::Version(cvi::model::MajorVersion_value, cvi::model::MinorVersion_value,
                          cvi::model::SubMinorVersion_value);
  auto fbModelName = fbb.CreateString(model_name);
  auto fbBuildTime = fbb.CreateString(getStrOfCurrentTime());
  auto fbMlirVersion = fb_model->mlir_version() ?
                       fbb.CreateString(fb_model->mlir_version()):
                       fbb.CreateString("unknown");
  auto fbTarget = fbb.CreateString(fb_model->target());

  auto fbWeightMap = rebuildWeightMap();
  auto fbSections = rebuildSections(tmp_dir, model_name, enc_buf);
  auto fbProgram = rebuildPrograms();
  auto encryptModel =
      cvi::model::CreateModel(fbb, &modelVersion, fbModelName, fbBuildTime,  0, 0,
                              fbWeightMap, fbProgram, fbSections, fbTarget, fbMlirVersion);
  fbb.Finish(encryptModel);

  std::vector<uint8_t> total_buf;
  total_buf.insert(total_buf.end(), fbb.GetBufferPointer(),
                   fbb.GetBufferPointer() + fbb.GetSize());
  total_buf.insert(total_buf.end(), enc_buf.data(), enc_buf.data() + enc_buf.size());
  cvi::runtime::MODEL_HEADER header;
  std::string magic = u8"CviModel";
  std::string padding = u8"AA";
  memcpy(header.magic, magic.c_str(), sizeof(header.magic));
  memcpy(header.padding, padding.c_str(), sizeof(header.padding));
  memset(header.chip, 0, 16);
  memcpy(header.chip, this->header.chip, sizeof(header.chip));
  header.body_size = fbb.GetSize();
  header.major = cvi::model::MajorVersion_value; // defined in cvimodel.fbs
  header.minor = cvi::model::MinorVersion_value; // defined in cvimodel.fbs

  std::ofstream ofs(output, std::ios::out | std::ios::binary);
  ofs.write(reinterpret_cast<char *>(&header), sizeof(header));
  ofs.write(reinterpret_cast<char *>(total_buf.data()), total_buf.size());
  ofs.close();

  // delete tmp_dir and file
  for (auto cmdbuf : cmdbuf_names) {
    std::string cmdbuf_file = tmp_dir + "/" + cmdbuf;
    std::string enc_cmdbuf = cmdbuf_file + ".enc";
    remove(cmdbuf_file.c_str());
    remove(enc_cmdbuf.c_str());
  }
  for (auto dmabuf : dmabuf_names) {
    std::string dmabuf_file = tmp_dir + "/" + dmabuf;
    std::string enc_cmdbuf = dmabuf_file + ".enc";
    remove(dmabuf_file.c_str());
    remove(enc_cmdbuf.c_str());
  }

  std::string weight_file = tmp_dir + "/" + weight_name;
  std::string enc_weight = weight_file + ".enc";
  remove(weight_file.c_str());
  remove(enc_weight.c_str());

  rmdir(tmp_dir.c_str());
}

size_t Model::compressSectionToFile(const cvi::model::Section *section, std::string dst) {
#ifdef ENABLE_COMPRESS_CMDBUF
  auto offset = section->offset();
  auto size = section->size();
  std::ofstream of(dst, std::ofstream::out | std::ofstream::binary |
                   std::ofstream::trunc);
  uint8_t *in_buf = new (std::nothrow) uint8_t[size];
  if (!in_buf) {
    printf("Failed to allocate buffer buff size:%d\n", size);
    exit(1);
  }
  auto got = stream.read(in_buf, binary_offset + offset, size);
  if (got != size) {
    printf("Failed to read data from cvimodel cmdbuf sections\n");
    exit(1);
  }
  // if compressed, exit
  if (section->compress()) {
    printf("cmdbuf or dmabuf already compressed! exit\n");
    exit(1);
  }
  size_t max_out_size = LZ4_compressBound(size);
  std::vector<uint8_t> out_buf(max_out_size);

  auto out_size = LZ4_compress_default(
      reinterpret_cast<char *>(in_buf),
      reinterpret_cast<char *>(out_buf.data()), size, max_out_size);
  if (out_size < 1) {
    printf("compress cmdbuf failed!\n");
    exit(1);
  }
  printf("compress cmdbuf, %d => %d\n", size, out_size);
  if (out_size > (int)size) {
    printf("compressed size large than decompressed size don't need compress!\n");
    exit(1);
  }
  of.write((const char *)out_buf.data(), out_size);
  of.close();
  delete[] in_buf;
  printf("store section to %s\n", dst.c_str());
  return size;
#else
  printf("Compressed section is not supported! please recompile with ENABLE_COMPRESS_CMDBUF\n");
  exit(1);
#endif
}

FBSectionVector Model::rebuildSections(
    std::string tmp_dir, std::string model_name,
    std::vector<uint8_t> &out_buf,
    std::map<std::string, size_t> &cmpr_info) {
  uint32_t offset = 0;
  uint32_t out_offset = 0;
  std::vector<flatbuffers::Offset<cvi::model::Section>> section_vec;
  for (auto s : *fb_model->sections()) {
    if (s->type() == cvi::model::SectionType_CMDBUF ||
        s->type() == cvi::model::SectionType_DMABUF) {
      std::string filename = tmp_dir + "/" + model_name +
                             "_" + s->name()->c_str() + ".bin";
      std::ifstream ifs(filename, std::ios::in | std::ios::binary);
      ifs.seekg(0, ifs.end);
      size_t size = ifs.tellg();
      ifs.seekg(0, ifs.beg);
      printf("filename: %s, filesize: %d\n", filename.c_str(), (int)size);
      char *buf = new char[size];
      if (buf == nullptr) {
        printf("alloc buf fail\n");
        exit(-1);
      }
      ifs.read(buf, size);
      out_buf.insert(out_buf.end(), buf, buf + size);
      delete[] buf;
      ifs.close();
      remove(filename.c_str());

      auto section = cvi::model::CreateSectionDirect(fbb, s->type(), s->name()->c_str(),
                                                     size, out_offset, false, true,
                                                     cmpr_info[s->name()->str()]);
      section_vec.push_back(section);
      out_offset += size;
    } else {
      uint8_t *buf = new uint8_t[s->size()];
      if (buf == nullptr) {
        printf("alloc buf fail\n");
        exit(-1);
      }
      stream.read(buf, binary_offset + offset, s->size());
      out_buf.insert(out_buf.end(), buf, buf + s->size());
      delete[] buf;

      auto section = cvi::model::CreateSectionDirect(fbb, s->type(), s->name()->c_str(),
                                                     s->size(), out_offset, false);
      section_vec.push_back(section);
      out_offset += s->size();
    }
    offset += s->size();
  }

  return fbb.CreateVector(section_vec);
}

void Model::compress_inst(std::string &output) {
  // create tmp dir to keep extracted cmdbuf and weight
  std::string tmp_dir = ".tmp_dir";
  struct stat file_stat;
  if (stat(tmp_dir.c_str(), &file_stat) != 0) {
    mkdir(tmp_dir.c_str(), 0777);
  } else {
    printf("create tmp directory fail: file exist!\n");
    return;
  }

  auto model_name = fb_model->name()->str();
  std::map<std::string, size_t> cmdbuf_compr_info;
  std::string weight_name;

  for (auto p : *fb_model->programs()) {
    for (auto r : *p->routines()) {
      if (r->type() != cvi::model::RoutineType_TPU)
        continue;

      std::string name;
      if (r->tpu_routine()->cmdbuf_section()) {
        name = r->tpu_routine()->cmdbuf_section()->str();
      } else if (r->tpu_routine()->dmabuf_section()) {
        name = r->tpu_routine()->dmabuf_section()->str();
      } else {
        assert(0);
      }
      printf("routine #%s\n", name.c_str());

      // compress cmdbuf
      for (auto s : *fb_model->sections()) {
        if (s->name()->str() == name) {
          std::string dst = model_name + "_" + s->name()->str() + ".bin";
          auto cmpr_sz = compressSectionToFile(s, std::string(tmp_dir + "/" + dst));
          cmdbuf_compr_info[s->name()->str()] = cmpr_sz;
          break;
        }
      }
    }
  }

  std::vector<uint8_t> enc_buf;
  cvi::model::Version modelVersion =
      cvi::model::Version(cvi::model::MajorVersion_value, cvi::model::MinorVersion_value,
                          cvi::model::SubMinorVersion_value);
  auto fbModelName = fbb.CreateString(model_name);
  auto fbBuildTime = fbb.CreateString(getStrOfCurrentTime());
  auto fbMlirVersion = fb_model->mlir_version() ?
                       fbb.CreateString(fb_model->mlir_version()):
                       fbb.CreateString("unknown");
  auto fbTarget = fbb.CreateString(fb_model->target());
  auto fbWeightMap = rebuildWeightMap();
  auto fbSections = rebuildSections(tmp_dir, model_name, enc_buf, cmdbuf_compr_info);
  auto fbProgram = rebuildPrograms();
  auto encryptModel =
      cvi::model::CreateModel(fbb, &modelVersion, fbModelName, fbBuildTime,  0, 0,
                              fbWeightMap, fbProgram, fbSections, fbTarget, fbMlirVersion);
  fbb.Finish(encryptModel);

  std::vector<uint8_t> total_buf;
  total_buf.insert(total_buf.end(), fbb.GetBufferPointer(),
                   fbb.GetBufferPointer() + fbb.GetSize());
  total_buf.insert(total_buf.end(), enc_buf.data(), enc_buf.data() + enc_buf.size());
  cvi::runtime::MODEL_HEADER header;
  std::string magic = u8"CviModel";
  std::string padding = u8"AA";
  memcpy(header.magic, magic.c_str(), sizeof(header.magic));
  memcpy(header.padding, padding.c_str(), sizeof(header.padding));
  memset(header.chip, 0, 16);
  memcpy(header.chip, this->header.chip, sizeof(header.chip));
  header.body_size = fbb.GetSize();
  header.major = cvi::model::MajorVersion_value; // defined in cvimodel.fbs
  header.minor = cvi::model::MinorVersion_value; // defined in cvimodel.fbs

  std::ofstream ofs(output, std::ios::out | std::ios::binary);
  ofs.write(reinterpret_cast<char *>(&header), sizeof(header));
  ofs.write(reinterpret_cast<char *>(total_buf.data()), total_buf.size());
  ofs.close();

  // delete tmp_dir
  rmdir(tmp_dir.c_str());
}

int main(int argc, const char **argv) {
  std::cout << argv[0] << "\n";
  showRuntimeVersion();

  argparse::ArgumentParser parser;
  parser.addArgument("-a", "--action", 1, false); // required
  parser.addArgument("-i", "--input", '+');       // inference count
  parser.addArgument("-o", "--output", 1);
  parser.parse(argc, argv);

  auto action = parser.retrieve<std::string>("action");
  auto inputs = parser.retrieve<std::vector<std::string>>("input");
  auto model = std::make_shared<Model>(inputs[0]);
  if (action == "dump") {
    assert(inputs.size() == 1);
    model->dump();
  } else if (action == "extract") {
    assert(inputs.size() == 1);
    model->extract();
  } else if (action == "compress") {
    assert(inputs.size() == 1);
    auto output = parser.retrieve<std::string>("output");
    if (output.empty()) {
      printf("ERROR: Please set output cvimodel\n");
      exit(1);
    }
    model->compress_inst(output);
  } else if (action == "encrypt") {
    assert(inputs.size() == 2);
    auto encrypter = inputs[1];
    auto output = parser.retrieve<std::string>("output");
    if (output.empty()) {
      printf("ERROR: Please set output cvimodel\n");
      exit(1);
    }
    model->encrypt(encrypter, output);
  } else if (action == "merge") {
    if (inputs.size() < 2) {
      printf("ERROR: Please set more than one cvimodels\n");
      exit(1);
    }
    auto output = parser.retrieve<std::string>("output");
    if (output.empty()) {
      printf("ERROR: Please set output cvimodel\n");
      exit(1);
    }
    std::vector<std::shared_ptr<Model>> models;
    models.push_back(model);
    for (int i = 1; i < (int)inputs.size(); i++) {
      auto other = std::make_shared<Model>(inputs[i]);
      models.push_back(other);
    }
    model->merge(models, output);
  }
  return 0;
}

