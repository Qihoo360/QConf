#CMakelist root
cmake_minimum_required (VERSION 2.6)
project (qconf-monitor)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "/usr/local/qconf/monitor" CACHE PATH "qconf_mointor install prefix" FORCE
        )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_subdirectory(src)

install(DIRECTORY bin DESTINATION .)
install(DIRECTORY conf DESTINATION .)
