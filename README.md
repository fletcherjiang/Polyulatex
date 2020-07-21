# acl

#### 介绍
ACL作为AI计算语言开发和运行平台，提供了Device管理、Context管理、Stream管理、内存管理、模型加载与执行、算子加载与执行、媒体数据处理等API，供用户开发AI应用，实现目标识别、图像分类、语言和文字处理等功能。

#### 软件架构
软件架构说明


#### 安装教程
ACL支持由源码编译，进行源码编译前，首先确保你有昇腾310AI处理器的环境，同时系统满足以下要求：

GCC >= 7.3.0
CMake >= 3.14.0
Autoconf >= 2.64
Libtool >= 2.4.6
Automake >= 1.15.1

1.  下载ACL源码
    ACL源码托管在码云平台，可由此下载。
    git clone https://gitee.com/cann/acl.git
    cd acl
2.  在ACL根目录下执行下列命令即可进行编译
    bash build.sh
    开始编译之前，请确保正确设置相关的环境变量。
    在build.sh的脚本中，默认会8线程编译，如果机器性能较差，可能会编译失败。可以通过-j{线程数}来控制线程数，如bash build.sh –j4。
3.  完成编译后，相应的动态库文件会生成在output文件夹中。

更多指令帮助，可以使用：
bash build.sh –h
如果想清除历史编译记录，可以如下操作：
rm -rf build/ output/
bash build.sh

#### 使用说明

1.  xxxx
2.  xxxx
3.  xxxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
