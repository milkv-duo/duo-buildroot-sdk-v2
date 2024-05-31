#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <mutex>
#include <linux/memfd.h>
#include <asm/unistd.h>
#include <runtime/debug.h>
#include <runtime/section.hpp>
#include <runtime/stream.hpp>

#if defined(__aarch64__) || defined(__arm__) || (__GNUC__ < 6)
#include <sys/syscall.h>
#include <fcntl.h>

/*
 * Before glibc version 2.27 there was no wrapper for memfd_create(2),
 * so we have to provide our own.
 *
 * Also define memfd fcntl sealing macros. While they are already
 * defined in the kernel header file <linux/fcntl.h>, that file as
 * a whole conflicts with the original glibc header <fnctl.h>.
 */

static inline int memfd_create(const char *name, unsigned int flags) {
  return syscall(SYS_memfd_create, name, flags);
}
#endif

namespace cvi {
namespace runtime {

bool CustomFunctionSection::load(BaseStream *stream, size_t offset, size_t size,
                                 std::vector<CpuRuntimeFunction *> &cpu_functions) {
  char path[64];
  uint8_t *buf = new uint8_t[size];
  if (!buf) {
    TPU_LOG_ERROR("Error, failed to allocate memory\n");
    return false;
  }
  stream->read(buf, offset, size);
  // load function by dlopen.
  shm_fd = memfd_create("cvitek", MFD_CLOEXEC);
  if (-1 == write(shm_fd, buf, size)) {
    TPU_LOG_ERROR("Error, write data to shared mem failed:%d\n", errno);
    delete[] buf;
    return false;
  }
  delete[] buf;

  snprintf(path, sizeof(path), "/proc/%d/fd/%d", getpid(), shm_fd);
  dso_handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!dso_handle) {
    TPU_LOG_ERROR("Error, dlopen %s, %s\n", path, dlerror());
    return false;
  }

  auto num = (int *)dlsym(dso_handle, "customOpRuntimeFuncsNum");
  if (!num) {
    TPU_LOG_ERROR("Error, dlsym find 'customOpRuntimeFuncsNum' failed\n");
    return false;
  }

  auto custom_funcs = (CustomOpRuntimeFunc*)dlsym(dso_handle, "customOpRuntimeFuncs");
  if (!custom_funcs) {
    TPU_LOG_ERROR("Error, dlsym find 'customOpRuntimeFuncs' failed\n");
    return false;
  }

  for (int i = 0; i < (*num); i++) {
    cpu_functions.push_back(
        new CpuRuntimeFunction(custom_funcs[i].name, custom_funcs[i].func));
  }
  return true;
}

CustomFunctionSection::~CustomFunctionSection() {
  if (dso_handle)
    dlclose(dso_handle);
  if (shm_fd)
    close(shm_fd);
}

} // namespace runtime
} // namespace cvi
