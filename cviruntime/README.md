# runtime

## overview

runtime is a lib released as SDK for use to develop TPU application. As well as a few tools for testing/benchmarking/profiling, etc.

tools

* test_cvimodel

## dependency

cvibuilder (for cvimodel_generated.h)
bmkernel (if run bmkernel directly)
cmodel (if RUNTIME=CMODEL)

## build

assuming install to ../install

assuming support install to ../install
assuming cvibuilder install to ../install
assuming bmkernel install to ../install
assuming cmodel install to ../install

```
$ cd runtime
$ mkdir build
$ cd build
$ cmake -G Ninja -DCHIP=BM1880v2 -DRUNTIME=CMODEL -DSUPPORT_PATH=../install -DCVIBUILDER_PATH=../install -DCVIKERNEL_PATH=../install -DCMODEL_PATH=../install -DCMAKE_INSTALL_PREFIX=../../install ..

Build
$ cmake --build .
$ cmake --build . -- -v

Install
$ cmake --build . --target install
$ cmake --build . --target install -- -v

Test
$ cmake --build . --target test -- -v

Uninstall
$ xargs rm < install_manifest.txt
```

## output

## test

'''
$ cd runtime/build
# cp bmnet/tests/regression/build/bm1880v2/caffe/resnet50/BM1880v2_resnet50_1.bmodel
$ ./test/test_bmnet_bmodel \
    /data/release/bmnet_models/resnet50/int8/resnet50_input_1_3_224_224.bin \
    BM1880v2_resnet50_1.bmodel \
    BM1880v2_resnet50_1_output.bin \
    1 3 224 224
'''

## TODO

* add SAFETY_FLAGS back
* for bm1880v2 only, need refactor for all chips
* add cpu layer back (comments out for now, search for SKIP_CPU_LAYER)
