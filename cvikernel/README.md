# bmkernel

## overview

bmkernel is a lib for TPU instruction generation, serving as assembly.

## dependency

none

## build

assuming install to ../install_bmkernel

```
$ cd bmkernel
$ mkdir build
$ cd build
$ cmake -G Ninja -DCHIP=BM1880v2 -DCMAKE_INSTALL_PREFIX=../../install_bmkernel ..

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

```
├── bin
│   └── readcmdbuf
├── include
│   └── bmkernel
│       ├── bm1880v2
│       │   └── bmkernel_1880v2.h
│       ├── bm_kernel.h
│       └── bm_kernel_legacy.h
└── lib
    ├── libbmkernel.so
    └── libbmkernel-static.a
```

## TODO

* add more testing
* mv assembly & disassembly here
* round trip testing, asm %s | disasm
