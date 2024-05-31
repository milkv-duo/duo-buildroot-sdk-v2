#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <runtime/debug.h>
#include <cvibuilder/cvimodel_generated.h>

void showRuntimeVersion() {
  printf("Cvitek Runtime (%d.%d.%d)%s\n",
         cvi::model::MajorVersion_value,
         cvi::model::MinorVersion_value,
         cvi::model::SubMinorVersion_value,
         RUNTIME_VERSION);
}

void dumpSysfsDebugFile(const char *path) {
  std::string line;
  std::ifstream file(path);
  std::cout << "dump " << path << "\n";
  while (std::getline(file, line )) {
    std::cout << line << "\n";
  }
  file.close();
  std::cout << "=======\n";
}

void mem_protect(uint8_t *vaddr, size_t size) {
  int ret = mprotect(vaddr, size, PROT_READ);
  if (ret != 0) {
    perror("cmdbuf memory protect failed");
  }  
}

void mem_unprotect(uint8_t *vaddr, size_t size) {
  int ret = mprotect(vaddr, size, PROT_READ | PROT_WRITE);
  if (ret != 0) {
    perror("cmdbuf memory unprotect failed");
  }  
}
