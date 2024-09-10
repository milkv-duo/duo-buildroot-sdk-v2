# 1 概述

使用sphinx来编写生成文档，以便于生成html和pdf版本的文档。

> 系列芯片技术文档

|芯片型号|存放位置|描述
|---|---|---|
|CV180系列 CV181系列|/CV180x_CV181x|已启用
|CV182系列|/CV182x|已启用
|CV183系列|/CV183x|已启用
|CV186系列|/CV186x|已启用

> **Note**
> - CV180x_CV181x芯片中文文档放在了 `CV180x_CV181x/zh` 下面
> - CV180x_CV181x芯片英文文档放在了 `CV180x_CV181x/en` 下面
> - CV182x芯片中文文档放在了 `CV182x/zh` 下面
> - CV182x芯片英文文档放在了 `CV182x/en` 下面
> - CV183x芯片中文文档放在了 `CV183x/zh` 下面
> - CV183x芯片英文文档放在了 `CV183x/en` 下面
> - CV186x芯片中文文档放在了 `CV186x/zh` 下面
> - CV186x芯片英文文档放在了 `CV186x/en` 下面

> **Note**
>
> 中文版权信息的声明页面对应 `common/zh_common/disclaimer_zh.rst` 文件
>
> 中文logo对应 `common/zh_common/images` 文件夹
>
> 如需修改`common/` 目录下内容，请联系 @[卢松沛](songpei.lu@sophgo.com)

# 2 开发环境配置

> 建议使用：Anaconda创建Python环境，使用Python3.8.12
>
> 自查方式：命令行输入  sphinx-build --version

# 3 快速创建文档

## 3.1 中文文档

```console
## 创建文档文件夹
mkdir -p <document_dir>
mkdir -p <document_dir>/source

## 复制Makefile模板到<document_dir>
cp -rf common/zh_templates/Makefile <document_dir>

## 复制index.rst模板、conf.py模板、1_disclaimer.rst模板到<document_dir>/source
cp -rf common/zh_templates/index.rst <document_dir>/source
cp -rf common/zh_templates/conf.py <document_dir>/source
cp -rf common/zh_templates/1_disclaimer.rst <document_dir>/source
```

1. 正常的初始化后的`<document_dir>`目录树如下：
```
├── Makefile
└── source
    ├── conf.py
    ├── disclaimer_zh.rst
    └── index.rst
```

2. 修改Makefile中的`module_name`和`output_file_prefix`

3. 修改conf.py中的`project`和`release`

4. 修改conf.py中的`<文档名称>`和`<版本>`和`<日期>`

5. 修改index.rst中的`<文档名称>`和`<版本>`和`<日期>`和`<描述>`

> **Note**
>
> conf.py中的`project` 字段需要和Makefile中的`module_name`对应上

## 3.2 英文文档

```console
## 创建文档文件夹
mkdir -p <document_dir>
mkdir -p <document_dir>/source

## 复制Makefile模板到<document_dir>
cp -rf common/en_templates/Makefile <document_dir>

## 复制index.rst模板、conf.py模板、1_disclaimer.rst模板到<document_dir>/source
cp -rf common/en_templates/index.rst <document_dir>/source
cp -rf common/en_templates/conf.py <document_dir>/source
cp -rf common/en_templates/1_disclaimer.rst <document_dir>/source
```

1. 正常的初始化后的文档文件夹目录树如下：
```
├── Makefile
└── source
    ├── conf.py
    ├── disclaimer_en.rst
    └── index.rst
```

2. 修改Makefile中的`module_name`和`output_file_prefix`

3. 修改conf.py中的`project`和`release`

4. 修改conf.py中的`<Document Name>`和`<Version>`和`<Date>`

5. 修改index.rst中的`<Document Name>`和`<Version>`和`<Date>`和`<Description>`

# 4 文档编译

```console
## 进入模块文档文件夹
cd <document_dir>

## 清空生成的文档
make clean

## 生成 html 文档
make html
 
## 生成 pdf 文档 
make pdf
```

# CV180x_CV181x芯片文档列表

|中文文档|英文文档|作者|校对|
|---|---|---|---|
|《TDL SDK软件开发指南》|《AI SDK Software Development Guide》|吴振杰|杨修|


# Built With

- [Sphinx](https://www.sphinx-doc.org/en/master/) -- Document auto generate tool
- [Latex](https://www.latex-project.org/) -- High-quality typesetting system

# License

This project is licensed under the MIT License - see the LICENSE file for details


