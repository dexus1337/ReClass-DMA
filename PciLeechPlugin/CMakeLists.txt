#/**
# * ----------------------------------------------------------------------
# * @file    CMakeLists.txt
# *
# * @brief   CMake Project file the Pcileech ReclassNET Plugin
# *
# * @author  dexus1337
# * ----------------------------------------------------------------------
# **/

cmake_minimum_required( VERSION 3.8 )


#/* Add Files to Compile */
add_library(PciLeechPlugin SHARED "dllmain.cpp" )

if ( ${CMAKE_CXX_FLAGS} MATCHES "^.*LINUX.*$" )

    add_custom_command(
        TARGET PciLeechPlugin PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/PciLeechPlugin/lib/vmm.so
        ${CMAKE_BINARY_DIR}/vmm.so
    )
    add_custom_command(
        TARGET PciLeechPlugin PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/PciLeechPlugin/lib/leechcore.so
        ${CMAKE_BINARY_DIR}/leechcore.so
    )

    target_link_libraries( PciLeechPlugin ":leechcore.so" )
    target_link_libraries( PciLeechPlugin ":vmm.so" )
endif ( )

if ( ${CMAKE_CXX_FLAGS} MATCHES "^.*_WIN32.*$" )

    add_custom_command(
        TARGET PciLeechPlugin PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/PciLeechPlugin/lib/vmm.lib
        ${CMAKE_BINARY_DIR}/vmm.lib
    )
    add_custom_command(
        TARGET PciLeechPlugin PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/PciLeechPlugin/lib/leechcore.lib
        ${CMAKE_BINARY_DIR}/leechcore.lib
    )

    target_link_libraries( PciLeechPlugin leechcore )
    target_link_libraries( PciLeechPlugin vmm )
endif ( )
