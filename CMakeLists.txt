cmake_minimum_required(VERSION 3.17)
project(DBServerConnect)
set(CMAKE_CXX_STANDARD 17)

#在目录中查找所有源文件
AUX_SOURCE_DIRECTORY(./ SRC_LIST)
AUX_SOURCE_DIRECTORY(./redis SRC_LIST)


SET(MYSQL_INCLUDE_DIR
        /usr/include/mysql
        /usr/local/include
        /usr/local/include/hiredis)
SET(MYSQL_LIB
        /usr/lib64/mysql
        /usr/local/lib
        /usr/local/lib64)

# 添加宏定义
ADD_DEFINITIONS( -g -W -Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DAC_HAS_INFO
        -DAC_HAS_WARNING -DAC_HAS_ERROR -DAC_HAS_CRITICAL -DTIXML_USE_STL
        -DAC_HAS_DEBUG -DLINUX_DAEMON -std=c++11 -DENCRYPT)

INCLUDE_DIRECTORIES(./ ./util  ${MYSQL_INCLUDE_DIR})
LINK_DIRECTORIES(./  ${MYSQL_LIB})
link_libraries(libglog.a)



add_executable(DBServerConnect main.cpp redis/CachePool.cpp redis/CachePool.h util/ThreadPool.cpp util/ThreadPool.h util/util.cpp util/util.h util/ConfigFileReader.cpp util/ConfigFileReader.h util/Thread.cpp util/Thread.h)


TARGET_LINK_LIBRARIES(
        DBServerConnect
        pthread
        dl
        libgflags.a
        libglog.a
        hiredis
        curl
        crypto)