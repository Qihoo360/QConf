#CMakelist driver
if (${HAS_PREFIX})
    set(DRIVER_COMPILE_FLAGS "-O2 -c -D__STDC_FORMAT_MACROS -DQCONF_INTERNAL -fPIC -g -pthread -Wall")
else()
    set(DRIVER_COMPILE_FLAGS "-O2 -c -D__STDC_FORMAT_MACROS -fPIC -g -pthread -Wall")
endif()
#set(LINK_FLAGS "-lpthread")

project(qconf_driver)
cmake_minimum_required(VERSION 2.6)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../../base
    ${PROJECT_SOURCE_DIR}/../../base/qlibc
    ${PROJECT_SOURCE_DIR}/../../deps/zookeeper/_install/include/zookeeper
    )

aux_source_directory(src DIR_SRCS)
aux_source_directory(../../base DIR_SRCS)
aux_source_directory(../../base/qlibc DIR_SRCS)

set_source_files_properties(../../base/qlibc/md5.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(../../base/qlibc/qhash.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(../../base/qlibc/qhasharr.c PROPERTIES LANGUAGE CXX )

file(GLOB ZK_OBJ_FILE
        "${PROJECT_SOURCE_DIR}/../../deps/zookeeper/_install/lib/*.o"
    )
message(${ZK_OBJ_FILE})

add_definitions(${DRIVER_COMPILE_FLAGS})
add_library(qconf_static STATIC ${DIR_SRCS}
    ${ZK_OBJ_FILE})

add_library(qconf SHARED ${DIR_SRCS})

# link libpthread
find_package (Threads)
target_link_libraries (qconf ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (qconf_static ${CMAKE_THREAD_LIBS_INIT})

# link static libzookeeper_mt
target_link_libraries(qconf
    ${PROJECT_SOURCE_DIR}/../../deps/zookeeper/_install/lib/libzookeeper_mt.a)

set_target_properties(qconf_static PROPERTIES OUTPUT_NAME qconf)
install(TARGETS qconf_static ARCHIVE DESTINATION lib)
install(TARGETS qconf LIBRARY DESTINATION lib)
install(DIRECTORY include DESTINATION .)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    install(CODE "EXECUTE_PROCESS (COMMAND ln -sf
    ${CMAKE_INSTALL_PREFIX}/lib/libqconf.so /usr/local/lib/libqconf.so)")
    install(CODE "EXECUTE_PROCESS (COMMAND ln -sf
    ${CMAKE_INSTALL_PREFIX}/lib/libqconf.a /usr/local/lib/libqconf.a)")
    install(DIRECTORY include/ DESTINATION /usr/local/include/qconf FILES_MATCHING PATTERN "*.h")
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
