#CMakelist src
cmake_minimum_required(VERSION 2.6)

set(MANAGER_COMPILE_FLAGS "-c -std=c++11 -fPIC -g -lpthread -Wall")

#build libzookeeper
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../deps/zookeeper COMMAND
        patch -f -p1 -i zookeeper-3.4.5.patch)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../deps/zookeeper COMMAND ./configure
    --prefix=${PROJECT_SOURCE_DIR}/../deps/zookeeper/_install)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../deps/zookeeper COMMAND
    make -j24)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../deps/zookeeper COMMAND
    make install)
execute_process(WORKING_DIRECTORY
    ${PROJECT_SOURCE_DIR}/../deps/zookeeper/_install/lib COMMAND
    ar x libzookeeper_mt.a)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../deps/zookeeper COMMAND
    cp src/zk_adaptor.h src/zk_hashtable.h _install/include/zookeeper)
include_directories(${PROJECT_SOURCE_DIR}/../deps/zookeeper/_install/include/zookeeper)

aux_source_directory(. DIR_SRCS)

file(GLOB ZK_OBJ_FILE
        "${PROJECT_SOURCE_DIR}/../deps/zookeeper/_install/lib/*.o"
    )
message(${ZK_OBJ_FILE})

add_definitions(${MANAGER_COMPILE_FLAGS})
add_executable(qconf_monitor ${DIR_SRCS})

# link libpthread
find_package (Threads)
target_link_libraries (qconf_monitor ${CMAKE_THREAD_LIBS_INIT})

# link static libzookeeper_mt
target_link_libraries(qconf_monitor
    ${PROJECT_SOURCE_DIR}/../deps/zookeeper/_install/lib/libzookeeper_mt.a)

install(TARGETS qconf_monitor DESTINATION bin)
