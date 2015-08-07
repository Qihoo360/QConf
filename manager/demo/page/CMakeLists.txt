#CMakelist demo/page
cmake_minimum_required(VERSION 2.6)
include_directories(
    cgic
    ${PROJECT_SOURCE_DIR}/../deps/zookeeper/_install/include/zookeeper
    ${PROJECT_SOURCE_DIR}/../src/c
    )
message(${PROJECT_SOURCE_DIR})
aux_source_directory(. DIR_SRCS)
aux_source_directory(cgic DIR_SRCS)

set_source_files_properties(cgic/cgic.c PROPERTIES LANGUAGE CXX )
add_executable(qconf_manage.cgi ${DIR_SRCS})

# link static libzookeeper_mt
target_link_libraries(qconf_manage.cgi qconf_zk_static)

install(DIRECTORY conf DESTINATION demo/www)
install(DIRECTORY css DESTINATION demo/www)
install(DIRECTORY js DESTINATION demo/www)
install(TARGETS qconf_manage.cgi DESTINATION demo/www)
install(FILES version DESTINATION demo/www)
