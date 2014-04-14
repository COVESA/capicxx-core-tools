# Copyright (C) 2014 BMW Group
# Author: Manfred Bathelt (manfred.bathelt@bmw.de)
# Author: Juergen Gehring (juergen.gehring@bmw.de)
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 2.8)

# Project settings
if (PRJ_NAME)
  message(STATUS "Start CMake for Project " ${PRJ_NAME})
else()
  message(FATAL_ERROR "Please specify your project name using -D PRJ_NAME=yourName")
endif()
project(${PRJ_NAME})
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "-Wall -O0 -std=c++0x")
set(CMAKE_BUILD_TYPE Debug)

# DBus Path
if (DBUS_LIB_PATH)
  message(STATUS "DBUS_LIB_PATH = " ${DBUS_LIB_PATH})
else()
  message(FATAL_ERROR "Please specify the path to your patched DBus library using -D DBUS_LIB_PATH=yourPath")
endif()

# CommonAPI
include(FindPkgConfig)
pkg_check_modules (DBUS "dbus-1 >= 1.4")
pkg_check_modules (COMMONAPI "CommonAPI >= 2.1")
pkg_check_modules (COMMONAPI_DBUS "CommonAPI-DBus >= 2.1")

# Source Files
set(PRJ_SRC_PATH src)
set(PRJ_SRC_GEN_PATH src-gen/commonapi/examples)

set(PRJ_NAME_CLIENT ${PROJECT_NAME}Client)
set(PRJ_NAME_SERVICE ${PROJECT_NAME}Service)

FILE(GLOB PRJ_PROXY_GEN_SRCS ${PRJ_SRC_GEN_PATH}/*Proxy.cpp ${PRJ_SRC_GEN_PATH}/*Types.cpp)
FILE(GLOB PRJ_STUB_GEN_SRCS ${PRJ_SRC_GEN_PATH}/*Stub*.cpp ${PRJ_SRC_GEN_PATH}/*Types.cpp)
FILE(GLOB PRJ_STUB_IMPL_SRCS ${PRJ_SRC_PATH}/*Stub*.cpp)

set(PRJ_CLIENT_SRCS ${PRJ_SRC_PATH}/${PRJ_NAME_CLIENT}.cpp ${PRJ_PROXY_GEN_SRCS})
set(PRJ_SERVICE_SRCS ${PRJ_SRC_PATH}/${PRJ_NAME_SERVICE}.cpp ${PRJ_STUB_GEN_SRCS} ${PRJ_STUB_IMPL_SRCS})

# Paths
message(STATUS "COMMONAPI_INCLUDEDIR = " ${COMMONAPI_INCLUDEDIR})
message(STATUS "COMMONAPI_DBUS_INCLUDEDIR = " ${COMMONAPI_DBUS_INCLUDEDIR})

include_directories(
    src-gen
    ${DBUS_INCLUDE_DIRS}
    ${COMMONAPI_INCLUDEDIR}
    ${COMMONAPI_DBUS_INCLUDEDIR}
)

link_directories(
    ${DBUS_LIB_PATH}
    ${COMMONAPI_LIBDIR}
    ${COMMONAPI_DBUS_LIBDIR}
)

# Build Client
add_executable(${PRJ_NAME_CLIENT} ${PRJ_CLIENT_SRCS})
target_link_libraries(${PRJ_NAME_CLIENT} CommonAPI CommonAPI-DBus)

# Build service
add_executable(${PRJ_NAME_SERVICE} ${PRJ_SERVICE_SRCS})
target_link_libraries(${PRJ_NAME_SERVICE} CommonAPI CommonAPI-DBus)
