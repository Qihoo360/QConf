#CMakelist root
project(qconf-manager)
cmake_minimum_required(VERSION 2.6)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "/usr/local/qconf/manage" CACHE PATH "qconf_manage install prefix" FORCE
        )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_subdirectory(src/c)
add_subdirectory(demo)
