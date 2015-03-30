#CMakelist root
set(HAS_PREFIX OFF CACHE BOOL "wether add prefix of qconf node on zookeeper")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "/usr/local/qconf" CACHE PATH "qconf install prefix" FORCE
        )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

cmake_minimum_required(VERSION 2.6)
#build libzookeeper
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/zookeeper COMMAND
    patch -f -p1 -i zookeeper-3.4.5.patch)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/zookeeper COMMAND ./configure
    --prefix=${PROJECT_SOURCE_DIR}/deps/zookeeper/_install)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/zookeeper COMMAND
    make)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/zookeeper COMMAND
    make install)
execute_process(WORKING_DIRECTORY
    ${PROJECT_SOURCE_DIR}/deps/zookeeper/_install/lib COMMAND
    ar x libzookeeper_mt.a)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/zookeeper COMMAND
    cp src/zk_adaptor.h src/zk_hashtable.h _install/include/zookeeper)

add_subdirectory(agent)
add_subdirectory(driver/c++)
add_subdirectory(driver/sh)
