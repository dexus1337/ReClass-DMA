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


#/* Link libraries */
if ( ${CMAKE_CXX_FLAGS} MATCHES "^.*LINUX.*$" )
    target_link_libraries( PciLeechPlugin ":leechcore.so" )
    target_link_libraries( PciLeechPlugin ":vmm.so" )
endif ( )

if ( ${CMAKE_CXX_FLAGS} MATCHES "^.*_WIN32.*$" )
    target_link_libraries( PciLeechPlugin leechcore )
    target_link_libraries( PciLeechPlugin vmm )
endif ( )
