#CMakelist sh

project(qconf_sh)
cmake_minimum_required(VERSION 2.6)
include_directories(
    ${PROJECT_SOURCE_DIR}/../c++/include
    )

aux_source_directory(. DIR_SRCS)

#add_definitions(${AGENT_COMPILE_FLAGS})
add_executable(qconf_client ${DIR_SRCS})

target_link_libraries (qconf_client qconf_static)

set_target_properties(qconf_client PROPERTIES OUTPUT_NAME qconf)
install(TARGETS qconf_client DESTINATION bin)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    install(CODE "EXECUTE_PROCESS (COMMAND ln -sf
    ${CMAKE_INSTALL_PREFIX}/bin/qconf /usr/local/bin/qconf)")
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
