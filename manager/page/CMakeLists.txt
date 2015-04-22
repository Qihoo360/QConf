cmake_minimum_required(VERSION 2.6)
include_directories(
    qlibc
    cgic
    ${PROJECT_SOURCE_DIR}/deps/zookeeper/_install/include/zookeeper
    )

aux_source_directory(. DIR_SRCS)
aux_source_directory(qlibc DIR_SRCS)
aux_source_directory(cgic DIR_SRCS)

set_source_files_properties(qlibc/md5.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(qlibc/qhash.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(cgic/cgic.c PROPERTIES LANGUAGE CXX )

add_definitions(${AGENT_COMPILE_FLAGS})
add_executable(qconf_manage.cgi ${DIR_SRCS})

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
    ${PROJECT_SOURCE_DIR}/../deps/zookeeper/_install COMMAND
    ar x libzookeeper_mt.a)
execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/zookeeper COMMAND
    cp src/zk_adaptor.h src/zk_hashtable.h _install/include/zookeeper)

# link libpthread
find_package (Threads)
target_link_libraries (qconf_manage.cgi ${CMAKE_THREAD_LIBS_INIT})

# link static libzookeeper_mt
target_link_libraries(qconf_manage.cgi
    ${PROJECT_SOURCE_DIR}/deps/zookeeper/_install/lib/libzookeeper_mt.a)

install(DIRECTORY conf DESTINATION www)
install(DIRECTORY css DESTINATION www)
install(DIRECTORY js DESTINATION www)
install(TARGETS qconf_manage.cgi DESTINATION www)
install(FILES version DESTINATION www)
