#!/bin/bash

# 如果脚本中的任何命令退出状态非零（即失败），那么脚本将立即终止执行（不会影响父shell）
set -e

SOURCE_DIR=`pwd`
SRC_BASE=${SOURCE_DIR}/src/base
SRC_NET=${SOURCE_DIR}/src/net

# 如果没有 build 目录 创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 如果没有 include 目录 创建该目录
if [ ! -d `pwd`/include ]; then
    mkdir `pwd`/include
fi

# 如果没有 lib 目录 创建该目录
if [ ! -d `pwd`/lib ]; then
    mkdir `pwd`/lib
fi

# 删除存在 build 目录生成文件并执行 cmake 命令
rm -fr ${SOURCE_DIR}/build/*
cd  ${SOURCE_DIR}/build &&
    cmake .. &&
    make install

# #回到项目根目录
# cd ..
# #把头文件拷贝到/usr/include/mymuduo so库拷贝到 /usr/lib  PATH
# if[ ! -d /usr/include/mymuduo ]; then
#     mkdir /usr/include/mymuduo
# fi

# for header in `ls *.h`
# do
#     cp $header /usr/include/mymuduo
# done

# 将头文件复制到 /usr/include
cp ${SOURCE_DIR}/include/mymuduo -r /usr/local/include 

# 将动态库文件复制到/usr/lib
cp ${SOURCE_DIR}/lib/libmymuduo.so /usr/local/lib

# 使操作生效
ldconfig