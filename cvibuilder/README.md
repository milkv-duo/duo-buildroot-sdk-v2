# cvibuilder

**先决条件：**
安装flatbuffers，https://github.com/google/flatbuffers

**安装：**
```
cmake -DFLATBUFFERS_PATH=/home/tpu/tpu/flatbuffers .
make
```

**输出：**
```
├── include
│   └── cvibuilder
│       ├── cvimodel_generated.h
│       └── parameter_generated.h
```