# runtime

## overview

runtime is a lib released as SDK for use to develop TPU application. As well as a few tools for testing/benchmarking/profiling, etc.

tools

* test_cvimodel

## dependency
ninja
flatbuffers（https://github.com/google/flatbuffers）
cvibuilder (for cvimodel_generated.h)（https://github.com/sophgo/cvibuilder）
cvikernel (if run cvikernel directly)（https://github.com/sophgo/cvibuilder）
cnpy （for cnpy.h）（https://github.com/wwwuxy/cnpy-for-tpu_mlir）
cmodel (if RUNTIME=CMODEL)

## build

assuming install to ../install

assuming support install to ../install
assuming flatbuffers install to ../flatbuffers
assuming cvibuilder install to ../cvibuild
assuming cvikernel install to ../cvikernel
assuming cnpy install to ../cnpy
assuming cmodel install to ../install

```
$ cd runtime
$ mkdir build
$ cd build
for cv系列芯片
$ cmake -G Ninja -DCHIP=cv181x -DRUNTIME=SOC -DFLATBUFFERS_PATH=../flatbuffers -DCVIBUILDER_PATH=../cvibuilder/build -DCVIKERNEL_PATH=../cvikernel/install_cvikernel -DCMAKE_INSTALL_PREFIX=../install ..

for bm系列芯片
$ cmake -G Ninja -DCHIP=BM1880v2 -DRUNTIME=SOC -DFLATBUFFERS_PATH=/../flatbuffers -DCVIBUILDER_PATH=/../cvibuilder/build -DCVIKERNEL_PATH=/../cvikernel/install_bmkernel -DCNPY_PATH=/../cnpy -DCMAKE_INSTALL_PREFIX=../install ..

Build
$ ninja

Install
$ ninja install

Test
$ cmake --build . --target test -- -v

Uninstall
$ xargs rm < install_manifest.txt
```

## output

```
install/lib/
├── libcviruntime.so
└── libcviruntime-static.a
```


## TODO

* add SAFETY_FLAGS back
* for bm1880v2 only, need refactor for all chips
* add cpu layer back (comments out for now, search for SKIP_CPU_LAYER)
