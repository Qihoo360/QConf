#CMakelist demo
project(qconf-manager-demo)
cmake_minimum_required(VERSION 2.6)

aux_source_directory(. SERVER_DIR_SRCS)
add_executable(web_server ${SERVER_DIR_SRCS})

add_subdirectory(page)

install(TARGETS web_server DESTINATION demo)
