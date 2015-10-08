#CMakelist test
project(qconf-test)
cmake_minimum_required(VERSION 2.6)

add_subdirectory(gtest)

add_definitions("-c -DQCONF_CURL_ENABLE -D__STDC_FORMAT_MACROS -g -pthread -Wall")

set(BASE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../base)
set(QLIBC_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../base/qlibc)
set(AGENT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../agent)
set(MANAGER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../manager/src/c)
set(ZK_SOURCE_DIR_PRE ${PROJECT_SOURCE_DIR}/../../deps/zookeeper)
set(ZK_SOURCE_DIR ${ZK_SOURCE_DIR_PRE}/_install)
set(CURL_SOURCE_DIR_PRE ${PROJECT_SOURCE_DIR}/../../deps/curl)
set(CURL_SOURCE_DIR ${CURL_SOURCE_DIR_PRE}/_install)
set(GDBM_SOURCE_DIR_PRE ${PROJECT_SOURCE_DIR}/../../deps/gdbm)
set(GDBM_SOURCE_DIR ${GDBM_SOURCE_DIR_PRE}/_install)
set(GTEST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/gtest)

#build libzookeeper
execute_process(WORKING_DIRECTORY ${ZK_SOURCE_DIR_PRE} COMMAND
    patch -f -p1 -i zookeeper-3.4.5.patch)
execute_process(WORKING_DIRECTORY ${ZK_SOURCE_DIR_PRE} COMMAND ./configure
    --prefix=${ZK_SOURCE_DIR})
execute_process(WORKING_DIRECTORY ${ZK_SOURCE_DIR_PRE} COMMAND
    make)
execute_process(WORKING_DIRECTORY ${ZK_SOURCE_DIR_PRE} COMMAND
    make install)
execute_process(WORKING_DIRECTORY
    ${ZK_SOURCE_DIR}/lib COMMAND
    ar x libzookeeper_mt.a)
execute_process(WORKING_DIRECTORY ${ZK_SOURCE_DIR_PRE} COMMAND
    cp src/zk_adaptor.h src/zk_hashtable.h _install/include/zookeeper)

#build libcurl
execute_process(WORKING_DIRECTORY ${CURL_SOURCE_DIR_PRE} COMMAND ./configure
    --prefix=${CURL_SOURCE_DIR}  --disable-ldap
    --disable-ldaps --without-ssl --without-zlib --without-libidn --without-librtmp)
execute_process(WORKING_DIRECTORY ${CURL_SOURCE_DIR_PRE} COMMAND
    make)
execute_process(WORKING_DIRECTORY ${CURL_SOURCE_DIR_PRE} COMMAND
    make install)


#build libgdbm
execute_process(WORKING_DIRECTORY ${GDBM_SOURCE_DIR_PRE} COMMAND ./configure
    --prefix=${GDBM_SOURCE_DIR})
execute_process(WORKING_DIRECTORY ${GDBM_SOURCE_DIR_PRE} COMMAND
    make)
execute_process(WORKING_DIRECTORY ${GDBM_SOURCE_DIR_PRE} COMMAND
    make install)



include_directories(
    ${BASE_SOURCE_DIR}
    ${QLIBC_SOURCE_DIR}
    ${AGENT_SOURCE_DIR}
    ${MANAGER_SOURCE_DIR}
    ${ZK_SOURCE_DIR}/include/zookeeper
    ${CURL_SOURCE_DIR}/include
    ${GDBM_SOURCE_DIR}/include
    ${GTEST_SOURCE_DIR}/include
    )

aux_source_directory(${BASE_SOURCE_DIR} DIR_SRCS)
aux_source_directory(${QLIBC_SOURCE_DIR} DIR_SRCS)
aux_source_directory(${AGENT_SOURCE_DIR} DIR_SRCS)
list(REMOVE_ITEM DIR_SRCS "${AGENT_SOURCE_DIR}/qconf_agent.cc")

aux_source_directory(${MANAGER_SOURCE_DIR} DIR_SRCS)
list(REMOVE_ITEM DIR_SRCS "${MANAGER_SOURCE_DIR}/qconf_format.cc")
aux_source_directory(${ZK_SOURCE_DIR} DIR_SRCS)
aux_source_directory(. DIR_SRCS)

set_source_files_properties(${QLIBC_SOURCE_DIR}/md5.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(${QLIBC_SOURCE_DIR}/qhash.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(${QLIBC_SOURCE_DIR}/qhasharr.c PROPERTIES LANGUAGE CXX )

add_executable(qconf_test ${DIR_SRCS})

# link libpthread
find_package (Threads)
target_link_libraries (qconf_test ${CMAKE_THREAD_LIBS_INIT})

# link gtest
target_link_libraries(qconf_test gtest)
target_link_libraries(qconf_test gtest_main)

# link libcurl
target_link_libraries(qconf_test ${CURL_SOURCE_DIR}/lib/libcurl.a)
# link static libgdbm
target_link_libraries(qconf_test ${GDBM_SOURCE_DIR}/lib/libgdbm.a)
# link static libzookeeper_mt
target_link_libraries(qconf_test ${ZK_SOURCE_DIR}/lib/libzookeeper_mt.a)

# link rt
IF(WIN32)
    MESSAGE(STATUS "This is windows")
ELSEIF(APPLE)
    MESSAGE(STATUS "This is apple")
ELSEIF(UNIX)
    MESSAGE(STATUS "This is unix, and add the librt.so")
    target_link_libraries (qconf_test rt)
ENDIF(WIN32)
