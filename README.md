[View English](./README.en.md)

<!-- TOC -->

- [acl](#acl)
    - [介绍](#介绍)
    - [安装教程](#安装教程)
    - [License](#license)

<!-- /TOC -->
# acl
## 介绍

ACL作为AI计算语言开发和运行平台，提供了Device管理、Context管理、Stream管理、内存管理、模型加载与执行、算子加载与执行、媒体数据处理等API，供用户开发AI应用，实现目标识别、图像分类、语言和文字处理等功能。


## 安装教程

ACL支持由源码编译，进行源码编译前，首先确保你有昇腾310AI处理器的环境，同时系统满足以下要求：

- GCC >= 7.3.0
- CMake >= 3.14.0
- Autoconf >= 2.64
- Libtool >= 2.4.6
- Automake >= 1.15.1

编译完成后会生成几个动态库，他们会链接到MindSpore中执行，无法单独运行。

1. 下载ACL源码
    ACL源码托管在码云平台，可由此下载。。
    ```
    git clone https://gitee.com/cann/acl.git
    cd acl
    ```

2. 在ACL根目录下执行下列命令即可进行编译。

    ```
    bash build.sh
    ```
    >  开始编译之前，请确保正确设置相关的环境变量。
    > - 在`build.sh`的脚本中，会进行`git clone`操作，请确保网络连接正常且git配置正确。
    > - 在`build.sh`的脚本中，默认会8线程编译，如果机器性能较差，可能会编译失败。可以通过`-j{线程数}`来控制线程数，如`bash build.sh –j4`。

3. 完成编译后，相应的动态库文件会生成在output文件夹中。

更多指令帮助，可以使用：
```
bash build.sh –h
```
如果想清除历史编译记录，可以如下操作：
```
rm -rf build/ output/
bash build.sh
```


## License

[Apache License 2.0](LICENSE)
