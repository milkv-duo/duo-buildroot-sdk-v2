#ifndef _BM_DEBUG_H_
#define _BM_DEBUG_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <memory.h>
#include <sys/mman.h>
#include "cvitpu_debug.h"


// print the version of runtime.
void showRuntimeVersion();
// dump sysfs debug file
void dumpSysfsDebugFile(const char *path);

void mem_protect(uint8_t *vaddr, size_t size);

void mem_unprotect(uint8_t *vaddr, size_t size);

#endif /* _BM_DEBUG_H_ */
