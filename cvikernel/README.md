# bmkernel

## overview

bmkernel is a lib for TPU instruction generation, serving as assembly.

## dependency

Need Install Ninja
```
sudo apt-get install ninja-build  # Ubuntu
sudo yum install ninja-build      # CentOS/RHEL
brew install ninja                # macOS 
```


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
- `-G Ninja`：使用 `Ninja` 作为构建系统生成器。如果不使用 `Ninja`，可以省略此参数，`CMake` 会使用默认的生成器（通常是 `Makefile`）。
- `-DCHIP=cv181x`：指定 `CHIP` 变量为 `cv181x`，从而选择特定的源文件和配置。
- `DCMAKE_INSTALL_PREFIX`：指定一个用户有写权限的安装目录

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
