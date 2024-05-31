cmdbuf param:
input1 : [1, k][uint8]
input2 : [n, k][uint8]
output : [n] [float32]

```
export MLIR_PATH=xxx
./build.sh

export SET_CHIP_NAME=cv183x
./build/megvii_euc
```