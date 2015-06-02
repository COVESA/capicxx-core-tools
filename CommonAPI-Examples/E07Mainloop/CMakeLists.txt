# Copyright (C) 2015 BMW Group
# Author: Lutz Bichler (lutz.bichler@bmw.de)
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 2.8)

set(PRJ_NAME E07Mainloop)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -O0 -std=c++0x -D_GLIBCXX_USE_NANOSLEEP -DLINUX")

message(STATUS "Compiler options: ${CMAKE_CXX_FLAGS}") 

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
       "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

OPTION(USE_INSTALLED_COMMONAPI "Set to OFF to use the local (build tree) version of CommonAPI" ON)
message(STATUS "USE_INSTALLED_COMMONAPI is set to value: ${USE_INSTALLED_COMMONAPI}")

if ("${USE_INSTALLED_COMMONAPI}" STREQUAL "ON")
    FIND_PACKAGE(CommonAPI REQUIRED CONFIG NO_CMAKE_PACKAGE_REGISTRY)
    FIND_PACKAGE(CommonAPI-DBus REQUIRED CONFIG NO_CMAKE_PACKAGE_REGISTRY)
else()
    FIND_PACKAGE(CommonAPI REQUIRED CONFIG NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
    FIND_PACKAGE(CommonAPI-DBus REQUIRED CONFIG NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
endif()

message(STATUS "CommonAPI_CONSIDERED_CONFIGS: ${CommonAPI_CONSIDERED_CONFIGS}")
message(STATUS "COMMONAPI_INCLUDE_DIRS: ${COMMONAPI_INCLUDE_DIRS}")
message(STATUS "CommonAPI-DBus_CONSIDERED_CONFIGS: ${CommonAPI-DBus_CONSIDERED_CONFIGS}")
message(STATUS "COMMONAPI_DBUS_INCLUDE_DIRS: ${COMMONAPI_DBUS_INCLUDE_DIRS}")

# CommonAPI
include(FindPkgConfig)
pkg_check_modules (DBUS "dbus-1 >= 1.4")

# SOME/IP
find_package (CommonAPI-SomeIP 3.1.1 REQUIRED)
find_package (vsomeip 1.1.0 REQUIRED)

# GLib
FIND_PACKAGE(glib-2.0)
if (GLIB_FOUND)
   message ( STATUS "Glib found: " ${GLIB_INCLUDE_DIRS} " " ${_GLIB_LIBRARY_DIR} " " ${GLIBCONFIG_INCLUDE_DIR} )
   include_directories(${GLIB_INCLUDE_DIRS} ${GLIBCONFIG_INCLUDE_DIR})
   #link_directories(${_GLIB_LIBRARY_DIR})
else (GLIB_FOUND)
   message ( WARNING "Glib not found.")
endif (GLIB_FOUND)

# Source Files
set(PRJ_SRC_PATH src)
set(PRJ_SRC_GEN_PATH src-gen)
set(PRJ_SRC_GEN_COMMONAPI_PATH ${PRJ_SRC_GEN_PATH}/core/v1_0/commonapi/examples)
set(PRJ_SRC_GEN_COMMONAPI_DBUS_PATH ${PRJ_SRC_GEN_PATH}/dbus/v1_0/commonapi/examples)
set(PRJ_SRC_GEN_COMMONAPI_SOMEIP_PATH ${PRJ_SRC_GEN_PATH}/someip/v1_0/commonapi/examples)

set(PRJ_NAME_CLIENT ${PRJ_NAME}Client)
set(PRJ_NAME_SERVICE ${PRJ_NAME}Service)

# Application
FILE(GLOB PRJ_PROXY_GEN_SRCS ${PRJ_SRC_GEN_COMMONAPI_PATH}/*Proxy.cpp)
FILE(GLOB PRJ_STUB_GEN_SRCS ${PRJ_SRC_GEN_COMMONAPI_PATH}/*Stub*.cpp)
FILE(GLOB PRJ_STUB_IMPL_SRCS ${PRJ_SRC_COMMONAPI_PATH}/*Stub*.cpp)
set(PRJ_CLIENT_SRCS ${PRJ_SRC_PATH}/${PRJ_NAME_CLIENT}.cpp ${PRJ_PROXY_GEN_SRCS})
set(PRJ_SERVICE_SRCS ${PRJ_SRC_PATH}/${PRJ_NAME_SERVICE}.cpp ${PRJ_SRC_PATH}/${PRJ_NAME}StubImpl.cpp ${PRJ_STUB_GEN_SRCS} ${PRJ_STUB_IMPL_SRCS})

# DBus library
FILE(GLOB PRJ_DBUS_LIB_SRCS ${PRJ_SRC_GEN_COMMONAPI_DBUS_PATH}/*cpp)

# SOME/IP library
FILE(GLOB PRJ_SOMEIP_LIB_SRCS ${PRJ_SRC_GEN_COMMONAPI_SOMEIP_PATH}/*cpp)

# Paths
OPTION(USE_INSTALLED_DBUS "Set to OFF to use the local (patched) version of dbus" ON)
message(STATUS "USE_INSTALLED_DBUS is set to value: ${USE_INSTALLED_DBUS}")

include_directories(
    src-gen/core
    src-gen/dbus
    src-gen/someip
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_DBUS_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
)

if ("${USE_INSTALLED_DBUS}" STREQUAL "ON")
link_directories(
    ${COMMONAPI_LIBDIR}
    ${COMMONAPI_DBUS_LIBDIR}
    ${COMMONAPI_SOMEIP_CMAKE_DIR}/build
)
else()
link_directories(
    ${COMMONAPI_LIBDIR}
    ${COMMONAPI_DBUS_LIBDIR}
    ${COMMONAPI_SOMEIP_CMAKE_DIR}/build
    ${DBUS_INCLUDE_DIRS}/dbus/.libs
)
endif()

set(LINK_LIBRARIES -Wl,--as-needed CommonAPI glib-2.0)

# Build Client
add_executable(${PRJ_NAME_CLIENT} ${PRJ_CLIENT_SRCS})
target_link_libraries(${PRJ_NAME_CLIENT} ${LINK_LIBRARIES})

# Build service
add_executable(${PRJ_NAME_SERVICE} ${PRJ_SERVICE_SRCS})
target_link_libraries(${PRJ_NAME_SERVICE} ${LINK_LIBRARIES})

# Build DBus library
add_library (${PRJ_NAME}-DBus SHARED ${PRJ_DBUS_LIB_SRCS})
target_link_libraries(${PRJ_NAME}-DBus CommonAPI-DBus)

# Build SOME/IP library
add_library (${PRJ_NAME}-SomeIP SHARED ${PRJ_SOMEIP_LIB_SRCS})
target_link_libraries(${PRJ_NAME}-SomeIP CommonAPI-SomeIP)