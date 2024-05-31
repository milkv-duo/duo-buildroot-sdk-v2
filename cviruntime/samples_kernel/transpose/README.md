### Setup and Build
1. tar -zxvf cvitek_mlir-ubuntu-18.04.tar.gz
2. tar -zxvf transpose.tar.gz
3. cd transpose
4. mkdir build && cd build
5. cmake -G Ninja -DTPU_BASE=/work ..
6. cmake --build .

### CviRuntime API
1. CVI_RC CVI_RT_Init(CVI_RT_HANDLE \*rt_handle)
  runtime初始化
  @param rt_handle runtime handle(context)

2. CVI_RT_MEM CVI_RT_MemAlloc(CVI_RT_HANDLE handle, uint64_t size)
  分配连续的运行时的device memory
  @param handle runtime handle
  @param size 分配的memory大小
  @ret 返回分配的device memory内存描述符

3. CVI_RT_MEM CVI_RT_MemPreAlloc(CVI_RT_MEM mem, uint64_t offset, uint64_t size)
  从分配的运行时memory中取一段内存
  @param mem device memory地址描述符
  @param offset 距离device memory物理地址首地址的偏移, 作为返回的memory的首地址
  @param size memory大小
  @ret 返回device memory的地址描述符

4. uint64_t CVI_RT_MemGetPAddr(CVI_RT_MEM mem)
  返回device memory的物理地址(用于cvikernel)
  @param mem device memory的地址描述符

5. uint8_t\* CVI_RT_MemGetVAddr(CVI_RT_MEM mem)
  返回device memory的虚拟地址(用于cpu寻址)
  @param mem device meory的地址描述符

6. CVI_RC CVI_RT_LoadCmdbuf(CVI_RT_HANDLE handle, uint8_t \*cmdbuf,
              uint64_t cmdbuf_sz, uint64_t gaddr_base0, uint64_t gaddr_base1,
              bool enable_pmu, CVI_RT_MEM *cmdbuf_mem)
  加载cmdbuf, 用来将cmdbuf从用户空间拷贝(vaddr)到内核空间(paddr)
  @param handle runtime handle
  @param cmdbuf cvikernel 生成的cmdbuf
  @param cmdbuf_sz cmdbuf的大小
  @param gaddr_base0 neuron tensor的base寄存器索引(0)
  @param gaddr_base1 weight的base寄存器索引(1)
  @param enable_pmu 是否使用pmu性能分析
  @param cmdbuf_mem 返回cmdbuf加载到device memory的地址

7. CVI_RC CVI_RT_RunCmdbuf(CVI_RT_HANDLE handle, CVI_RT_MEM cmdbuf_mem,
              uint64_t gaddr_base2, uint64_t gaddr_base3)
  运行cmdbuf
  @param handle runtime handle
  @param cmdbuf_mem cmdbuf的device memory地址描述符
  @param gaddr_base2 input tensor的device memory的物理地址
  @param gaddr_base3 output tensor的device memory的物理地址
8. CVI_RC CVI_RT_MemFlush(CVI_RT_HANDLE handle, CVI_RT_MEM mem)
  刷新cache
  @param handle runtime handle
  @param mem deivce memory地址描述符
9. CVI_RC CVI_RT_DeInit(CVI_RT_HANDLE handle)
  @param handle runtime handle

详情参考transpose.cpp注释
