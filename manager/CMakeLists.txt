#CMakelist root
project(qconf-manage)
cmake_minimum_required(VERSION 2.6)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "/usr/local/qconf/manage" CACHE PATH "qconf_manage install prefix" FORCE
        )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

aux_source_directory(. SERVER_DIR_SRCS)
add_executable(web_server ${SERVER_DIR_SRCS})

add_subdirectory(page)

install(TARGETS web_server DESTINATION .)
