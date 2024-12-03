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

project( PciLeechPlugin )

include_directories( ./PciLeechPlugin/ )

add_subdirectory( PciLeechPlugin )