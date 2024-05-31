#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "cviruntime.h"
#include <runtime/debug.h>
#include <cvikernel/cvikernel.h>
#include "cviruntime_context.h"

namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

struct PythonTensor {
  PythonTensor(CVI_TENSOR *tensor) {
    name = std::string(tensor->name);
    qscale = tensor->qscale;
    zpoint = tensor->zero_point;
    std::vector<size_t> shape;
    for (int i = 0; i < (int)tensor->shape.dim_size; i++) {
      shape.push_back(tensor->shape.dim[i]);
    }
    data = py::array(getDtype(tensor->fmt), shape, (void *)CVI_NN_TensorPtr(tensor),
                     py::cast(*this));
  }

  std::string name;
  float qscale;
  int zpoint;
  py::array data;

private:
  py::dtype getDtype(CVI_FMT fmt) {
    switch (fmt) {
      case CVI_FMT_FP32:
        return py::dtype("single");
      case CVI_FMT_INT8:
        return py::dtype("int8");
      case CVI_FMT_UINT8:
        return py::dtype("uint8");
      case CVI_FMT_INT16:
        return py::dtype("int16");
      case CVI_FMT_UINT16:
        return py::dtype("uint16");
      case CVI_FMT_INT32:
        return py::dtype("int32");
      case CVI_FMT_UINT32:
        return py::dtype("uint32");
      case CVI_FMT_BF16:
        // numpy has no bf16 type, use uint16 instread of bf16.
        return py::dtype("uint16");
      default:
        assert(0);
    }
  }
};

struct PythonCviModel {
  PythonCviModel(const std::string &model_file, int program_id, bool output_all_tensors) {
    int ret = CVI_NN_RegisterModel(model_file.c_str(), &model);
    if (ret != 0) {
      assert(0);
    }
    this->config(program_id, output_all_tensors);
  }

  ~PythonCviModel() { CVI_NN_CleanupModel(model); }

  py::object clone() {
    auto new_cvimodel = new PythonCviModel();
    int ret = CVI_NN_CloneModel(model, &new_cvimodel->model);
    if (ret != 0) {
      assert(0);
    }
    return py::cast(new_cvimodel);
  }

  void config(int program_id, bool output_all_tensors) {
    CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, program_id);
    CVI_NN_SetConfig(model, OPTION_OUTPUT_ALL_TENSORS, output_all_tensors);
    int32_t ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                         &output_tensors, &output_num);
    if (ret != 0) {
      assert(0);
    }
    for (int i = 0; i < input_num; i++) {
      inputs.push_back(std::make_shared<PythonTensor>(&input_tensors[i]));
    }
    for (int i = 0; i < output_num; i++) {
      outputs.push_back(std::make_shared<PythonTensor>(&output_tensors[i]));
    }
  }

  void forward() {
    int ret = CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
    if (ret != 0) {
      assert(0);
    }
  }

  std::vector<std::shared_ptr<PythonTensor>> inputs;
  std::vector<std::shared_ptr<PythonTensor>> outputs;

private:
  PythonCviModel() {}
  CVI_MODEL_HANDLE model = nullptr;
  int32_t input_num = 0;
  int32_t output_num = 0;
  CVI_TENSOR *input_tensors = nullptr;
  CVI_TENSOR *output_tensors = nullptr;
};

class PyCvkLmTensor {
public:
  PyCvkLmTensor() {}

  PyCvkLmTensor(cvk_context_t *cvk_ctx, cvk_tl_shape_t shape,
      cvk_fmt_t fmt, int eu_align) : cvk_ctx(cvk_ctx), fmt(fmt),
          eu_align(eu_align) {

    if (!cvk_ctx)
      throw std::runtime_error("Expect valid kernel context");

    lmTensor =
        cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt, eu_align);
    if (!lmTensor)
      throw std::runtime_error("Fail to allocate tensor in local memory");
  }

  std::vector<int> shapes() {
    std::vector<int> shapes = {0, 0, 0, 0};

    if (lmTensor) {
      shapes[0] = lmTensor->shape.n;
      shapes[1] = lmTensor->shape.c;
      shapes[2] = lmTensor->shape.h;
      shapes[3] = lmTensor->shape.w;
    }

    return shapes;
  }

  int address() {
    if (lmTensor)
      return static_cast<int>(lmTensor->start_address);

    return 0;
  }

  cvk_tl_t *allocated() {
    return lmTensor;
  }

  cvk_fmt_t format() {
    if (lmTensor)
      return lmTensor->fmt;

    return CVK_FMT_I8;
  }

private:
  cvk_context_t *cvk_ctx = nullptr;
  cvk_tl_shape_t shape;
  cvk_fmt_t fmt = CVK_FMT_I8;
  int eu_align = 0;
  cvk_tl_t *lmTensor = nullptr;
};

class PyCviKernelContext {
public:
  const uint32_t CMDBUF_SIZE = 512 * 1024;

  PyCviKernelContext(const std::string &name) : name(name) {
    CVI_RT_Init(&rt_handle);
    assert(rt_handle);
    submit_handle = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
    assert(submit_handle);
    cvk_ctx = (cvk_context_t *)submit_handle;
  }

  ~PyCviKernelContext() {
    CVI_RT_UnRegisterKernel(cvk_ctx);
    CVI_RT_DeInit(rt_handle);
  }

  cvk_fmt_t getCvkDataFormat(std::string format);

  void checkTdmaParameters(py::buffer b, PyCvkLmTensor *lmTensor);
  void setupGmTensor(cvk_tg_t &tg, py::buffer_info &info, CVI_RT_MEM mem);

  // Kernel Operations
  PyCvkLmTensor lmem_alloc_tensor(py::buffer b, int eu_align);
  void tdma_g2l_tensor_copy(PyCvkLmTensor *lmTensor, py::buffer b);
  void tdma_l2g_tensor_copy(py::buffer b, PyCvkLmTensor *lmTensor);

private:
  std::string name;
  CVI_RT_HANDLE rt_handle = nullptr;
  CVI_RT_KHANDLE submit_handle = nullptr;
  cvk_context_t *cvk_ctx = nullptr;
};

cvk_fmt_t PyCviKernelContext::getCvkDataFormat(std::string format) {
  if (format == "b")
    return CVK_FMT_I8;

  return CVK_FMT_I8;
}

void PyCviKernelContext::checkTdmaParameters(py::buffer b,
                                             PyCvkLmTensor *lmTensor) {
  if (!lmTensor)
    throw std::runtime_error("Tensor in Local memory not assigned");

  if (!lmTensor->allocated())
    throw std::runtime_error("Tensor in local memory not allocated yet");

  py::buffer_info info = b.request();
  if (info.ndim != 4)
    throw std::runtime_error("Only support NCHW 4D tensor");

  if ((info.shape[0] != lmTensor->shapes()[0]) ||
      (info.shape[1] != lmTensor->shapes()[1]) ||
      (info.shape[2] != lmTensor->shapes()[2]) ||
      (info.shape[3] != lmTensor->shapes()[3]))
    throw std::runtime_error("Shape mismatched");
}

void PyCviKernelContext::setupGmTensor(cvk_tg_t &tg, py::buffer_info &info,
                                       CVI_RT_MEM mem) {
  memset(&tg, 0, sizeof(tg));
  cvk_tg_shape_t tg_shape = {
      static_cast<uint32_t>(info.shape[0]),
      static_cast<uint32_t>(info.shape[1]),
      static_cast<uint32_t>(info.shape[2]),
      static_cast<uint32_t>(info.shape[3])};
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &tg, tg_shape,
                                 getCvkDataFormat(info.format));
  tg.start_address = CVI_RT_MemGetPAddr(mem);
}

PyCvkLmTensor PyCviKernelContext::lmem_alloc_tensor(py::buffer b,
                                                    int eu_align) {
  py::buffer_info info = b.request();

  if (info.ndim != 4)
    throw std::runtime_error("Local memory only support NCHW");

  if (!info.shape[0] || !info.shape[1] || !info.shape[2] || !info.shape[3])
    throw std::runtime_error("Shape should not zero");

  cvk_tl_shape_t shape = {
      static_cast<uint32_t>(info.shape[0]),
      static_cast<uint32_t>(info.shape[1]),
      static_cast<uint32_t>(info.shape[2]),
      static_cast<uint32_t>(info.shape[3])};

  cvk_fmt_t fmt = getCvkDataFormat(info.format);

  PyCvkLmTensor lmTensor(cvk_ctx, shape, fmt, eu_align);

  return lmTensor;
}

void PyCviKernelContext::tdma_g2l_tensor_copy(PyCvkLmTensor *lmTensor,
                                              py::buffer b) {
  checkTdmaParameters(b, lmTensor);

  if (!lmTensor)
    throw std::runtime_error("Tensor in Local memory not assigned");

  if (!lmTensor->allocated())
    throw std::runtime_error("Tensor in local memory not allocated yet");

  py::buffer_info info = b.request();

  size_t gm_size = info.shape[0] * info.shape[1] * info.shape[2] *
                   info.shape[3];
  CVI_RT_MEM mem = CVI_RT_MemAlloc(rt_handle, gm_size);

  // Copy from system memory to device memory
  CVI_RT_MemCopyS2D(rt_handle, mem, static_cast<uint8_t *>(info.ptr));

  // Setup global memory
  cvk_tg_t tg;
  setupGmTensor(tg, info, mem);

  cvk_tdma_g2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = lmTensor->allocated();
  cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &p);

  CVI_RT_Submit(cvk_ctx);

  // free the device memory
  CVI_RT_MemFree(rt_handle, mem);
}

void PyCviKernelContext::tdma_l2g_tensor_copy(py::buffer b,
                                              PyCvkLmTensor *lmTensor) {
  checkTdmaParameters(b, lmTensor);

  py::buffer_info info = b.request();

  size_t gm_size = info.shape[0] * info.shape[1] * info.shape[2] *
                   info.shape[3];
  CVI_RT_MEM mem = CVI_RT_MemAlloc(rt_handle, gm_size);

  // Setup global memory
  cvk_tg_t tg;
  setupGmTensor(tg, info, mem);

  cvk_tdma_l2g_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = lmTensor->allocated();
  p.dst = &tg;
  cvk_ctx->ops->tdma_l2g_tensor_copy(cvk_ctx, &p);
  CVI_RT_Submit(cvk_ctx);

  // Copy from device memory to system memory
  CVI_RT_MemCopyD2S(rt_handle, static_cast<uint8_t *>(info.ptr), mem);

  // Free the device memory
  CVI_RT_MemFree(rt_handle, mem);
}

PYBIND11_MODULE(pyruntime, m) {
  py::class_<PythonTensor, std::shared_ptr<PythonTensor>>(m, "Tensor")
      .def_readonly("name", &PythonTensor::name)
      .def_readonly("qscale", &PythonTensor::qscale)
      .def_readonly("zpoint", &PythonTensor::zpoint)
      .def_readwrite("data", &PythonTensor::data);

  py::class_<PythonCviModel>(m, "Model")
      .def(py::init<const std::string &, int, bool>(),
           py::arg("cvimodel"), py::arg("program_id") = 0,
           py::arg("output_all_tensors") = false)
      .def("forward", &PythonCviModel::forward)
      .def_readwrite("inputs", &PythonCviModel::inputs)
      .def_readwrite("outputs", &PythonCviModel::outputs);

  py::class_<PyCvkLmTensor>(m, "CvkLmTensor")
      .def(py::init<>())
      .def("shapes", &PyCvkLmTensor::shapes, "Get shape.")
      .def("address", &PyCvkLmTensor::address, "Get address.");

  py::class_<PyCviKernelContext>(m, "CvkContext")
      .def(py::init<const std::string &>())
      .def("lmem_alloc_tensor", &PyCviKernelContext::lmem_alloc_tensor,
           "Allocate tensor in TPU local memory.")
      .def("tdma_g2l_tensor_copy", &PyCviKernelContext::tdma_g2l_tensor_copy,
           "Transfer data from host to TPU local memory.")
      .def("tdma_l2g_tensor_copy", &PyCviKernelContext::tdma_l2g_tensor_copy,
           "Transfer data from TPU local memory to host.");
}
