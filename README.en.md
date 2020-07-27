[查看中文](./README.md)

<!-- TOC -->

- [acl](#acl)
    - [Description](#Description)
    - [Installation](#Installation)
    - [License](#license)


<!-- /TOC -->
# acl
## Description

As an AI computing language development and operation platform, ACL provides APIs such as Device management, Context management, Stream management, memory management, model loading and execution, operator loading and execution, media data processing, etc., for users to develop AI applications and achieve target recognition , Image classification, language and word processing functions.


## Installation

You can build GraphEngine from source.
To build acl, please make sure that you have access to an Ascend310 environment as compiling environment, and make sure that following software requirements are fulfilled.

- GCC >= 7.3.0
- CMake >= 3.14.0
- Autoconf >= 2.64
- Libtool >= 2.4.6
- Automake >= 1.15.1

The output of building GraphEngine is a set of shared libraries which can be linked with cann, they are not meant to be used independently.

1. Download acl source code
    GraphEngine source code is available on [Gitee](https://gitee.com/cann/acl):
    ```
    git clone https://gitee.com/cann/acl.git
    cd acl
    ```

2. Run the following command in the root directory of the source code to compile acl:
To build with default options, simply:

    ```
    bash build.sh
    ```
 > - Before running the preceding command, ensure that the relevant paths have been added to the environment variable PATH.
 > - In the build.sh script, the git clone command will be executed to obtain code from Gitee.com. Ensure that the network settings of Git are correct.
 > - In the build.sh script, the default number of compilation threads is 8. If the compiler performance is poor, compilation errors may occur. You can add -j{Number of threads} in to bash command to reduce the number of threads. For example, `bash build.sh -j4`.

3. Access the output directory of the source code, obtain the generated acl libraries which can be linked with acl for further installation/testing.

For more information on other options of building acl:
```
bash build.sh –h
```
If you wish to clean all outputs from last build and try again：
```
rm -rf build/ output/
bash build.sh
```


## License

[Apache License 2.0](LICENSE)
