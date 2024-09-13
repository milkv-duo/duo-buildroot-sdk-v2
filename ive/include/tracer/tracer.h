#ifndef TRACER_H
#define TRACER_H
/*
 * Source Code
 * https://android.googlesource.com/platform/frameworks/native/+/5b38a1d/include/utils/Trace.h
 * https://android.googlesource.com/platform/frameworks/native/+/jb-dev/libs/utils/Trace.cpp
 */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

class Tracer {
 public:
  inline Tracer() {
#if ENABLE_TRACE
    Init();
#endif
  }

  static inline void TraceCounter(const char* name, int32_t value) {
#if ENABLE_TRACE
    char buf[1024];
    snprintf(buf, 1024, "C|%d|%s|%d", getpid(), name, value);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(sTraceFD, buf, strlen(buf));
#pragma GCC diagnostic pop
#endif
  }

  static inline void TraceBegin(const char* name) {
#if ENABLE_TRACE
    char buf[1024];
    size_t len = snprintf(buf, 1024, "B|%d|%s", getpid(), name);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(sTraceFD, buf, len);
#pragma GCC diagnostic pop
#endif
  }

  static inline void TraceEnd() {
#if ENABLE_TRACE
    char buf = 'E';
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(sTraceFD, &buf, 1);
#pragma GCC diagnostic pop
#endif
  }

 private:
  static inline void Init() {
#if ENABLE_TRACE
    const char* const traceFileName = "/sys/kernel/debug/tracing/trace_marker";
    sTraceFD = open(traceFileName, O_WRONLY);
    if (sTraceFD == -1) {
      std::cout << "error opening trace file: " << strerror(errno) << " (" << errno << ")"
                << std::endl;
      // sEnabledTags remains zero indicating that no tracing can occur
    }
#endif
  }

#if ENABLE_TRACE
  static int sTraceFD;
#endif
};

class ScopedTrace {
 public:
  inline ScopedTrace(const char* name) {
#if ENABLE_TRACE
    Tracer::TraceBegin(name);
#endif
  }

  inline ~ScopedTrace() {
#if ENABLE_TRACE
    Tracer::TraceEnd();
#endif
  }
};
#endif  // TRACER_H
