#CMakelist src
cmake_minimum_required(VERSION 2.6)

set(MANAGER_COMPILE_FLAGS "-c -fPIC -g -pthread -Wall")

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
include_directories(
    qlibc
    ${PROJECT_SOURCE_DIR}/deps/zookeeper/_install/include/zookeeper
    )

aux_source_directory(. DIR_SRCS)
aux_source_directory(qlibc DIR_SRCS)

set_source_files_properties(qlibc/md5.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(qlibc/qhash.c PROPERTIES LANGUAGE CXX )

file(GLOB ZK_OBJ_FILE
        "${PROJECT_SOURCE_DIR}/deps/zookeeper/_install/lib/*.o"
    )
message(${ZK_OBJ_FILE})

add_definitions(${MANAGER_COMPILE_FLAGS})
add_library(qconf_zk_static STATIC ${DIR_SRCS}
    ${ZK_OBJ_FILE})
add_library(qconf_zk SHARED ${DIR_SRCS})

# link libpthread
find_package (Threads)
target_link_libraries (qconf_zk ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (qconf_zk_static ${CMAKE_THREAD_LIBS_INIT})

# link static libzookeeper_mt
target_link_libraries(qconf_zk
    ${PROJECT_SOURCE_DIR}/deps/zookeeper/_install/lib/libzookeeper_mt.a)

set_target_properties(qconf_zk_static PROPERTIES OUTPUT_NAME qconf_zk)
install(TARGETS qconf_zk DESTINATION lib)
install(TARGETS qconf_zk_static DESTINATION lib)
install(FILES qconf_zk.h DESTINATION include)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/deps/zookeeper/_install/include/zookeeper DESTINATION include)
