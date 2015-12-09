cmake_minimum_required (VERSION 2.8.1)

PROJECT(capi_verification_tests)

message("Project name: ${PROJECT_NAME}")

##############################################################################

# create the commandline variables:
OPTION(USE_INSTALLED_COMMONAPI "Set to OFF to use the local (build tree) version of CommonAPI" ON)
message("USE_INSTALLED_COMMONAPI is set to value: ${USE_INSTALLED_COMMONAPI}")

SET(CMAKE_GLUECODE_DBUS_NAME "na" CACHE STRING "The cmake name of the gluecode library without cmake extension e.g. DBusGlue.")
if("${CMAKE_GLUECODE_DBUS_NAME}" STREQUAL "na")
  message(WARNING "The cmake name of the D-Bus gluecode library without cmake extension needs to be specified. Use '-DCMAKE_GLUECODE_DBUS_NAME=' to do so.")
else()
  message("CMAKE_GLUECODE_DBUS_NAME specified: ${CMAKE_GLUECODE_DBUS_NAME}")
endif()

SET(CMAKE_GLUECODE_SOMEIP_NAME "na" CACHE STRING "The cmake name of the gluecode library without cmake extension e.g. DBusGlue.")
if("${CMAKE_GLUECODE_SOMEIP_NAME}" STREQUAL "na")
  message(WARNING "The cmake name of the SomeIP gluecode library without cmake extension needs to be specified. Use '-DCMAKE_GLUECODE_SOMEIP_NAME=' to do so.")
else()
  message("CMAKE_GLUECODE_SOMEIP_NAME specified: ${CMAKE_GLUECODE_SOMEIP_NAME}")
endif()

if("${CMAKE_GLUECODE_DBUS_NAME}" STREQUAL "na" AND "${CMAKE_GLUECODE_SOMEIP_NAME}" STREQUAL "na")
    message(FATAL_ERROR "Please specify at least one of CMAKE_GLUECODE_DBUS_NAME or CMAKE_GLUECODE_SOMEIP_NAME")
    return()
endif()

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
       "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

message("Build type: ${CMAKE_BUILD_TYPE}")

##############################################################################
# add COMMONAPI install path to cmake find root path

message("CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")

FIND_PACKAGE(PkgConfig)

# find gluecode for D-Bus
if(NOT "${CMAKE_GLUECODE_DBUS_NAME}" STREQUAL "na")
    FIND_PACKAGE(${CMAKE_GLUECODE_DBUS_NAME})
    if ("${${CMAKE_GLUECODE_DBUS_NAME}_CONSIDERED_CONFIGS}" STREQUAL "")
      message(WARNING "The cmake file for the D-Bus gluecode was not found. Please DBusGlue_DIR. Use '-DDBusGlue_DIR=' to do so.")
    else()
      message("${CMAKE_GLUECODE_DBUS_NAME}_CONSIDERED_CONFIGS found: ${${CMAKE_GLUECODE_DBUS_NAME}_CONSIDERED_CONFIGS}")
    endif()
endif()

# find gluecode for SomeIP
if(NOT "${CMAKE_GLUECODE_SOMEIP_NAME}" STREQUAL "na")
    FIND_PACKAGE(${CMAKE_GLUECODE_SOMEIP_NAME})
    if ("${${CMAKE_GLUECODE_SOMEIP_NAME}_CONSIDERED_CONFIGS}" STREQUAL "")
      message(WARNING "The cmake file for the SomeIP gluecode was not found. Please specify SomeIPGlue_DIR. Use '-DSomeIPGlue_DIR=' to do so.")
    else()
      message("${CMAKE_GLUECODE_SOMEIP_NAME}_CONSIDERED_CONFIGS found: ${${CMAKE_GLUECODE_SOMEIP_NAME}_CONSIDERED_CONFIGS}")
    endif()
endif()

# find commonapi
if ("${USE_INSTALLED_COMMONAPI}" STREQUAL "ON")
    FIND_PACKAGE(CommonAPI REQUIRED CONFIG NO_CMAKE_PACKAGE_REGISTRY)
else()
    FIND_PACKAGE(CommonAPI REQUIRED CONFIG NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
endif()

message("CommonAPI_CONSIDERED_CONFIGS: ${CommonAPI_CONSIDERED_CONFIGS}")
message("COMMONAPI_INCLUDE_DIRS: ${COMMONAPI_INCLUDE_DIRS}")

##############################################################################
# generate code from fidl
file(GLOB FIDL_FILES "fidl/*.fidl")
execute_process(COMMAND ${COMMONAPI_TOOL_GENERATOR} --dest src-gen --skel ${FIDL_FILES}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        )

##############################################################################
# get library names

string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_CMAKE_BUILD_TYPE)
message("UPPER_CMAKE_BUILD_TYPE: ${UPPER_CMAKE_BUILD_TYPE}")

# libDBusGLue
if(NOT "${CMAKE_GLUECODE_DBUS_NAME}" STREQUAL "na")
    get_property(GLUECODE_LIB_PATH_NAME_DBUS TARGET ${CMAKE_GLUECODE_DBUS_NAME} PROPERTY IMPORTED_LOCATION_${UPPER_CMAKE_BUILD_TYPE})
    message("GLUECODE_LIB_PATH_NAME_DBUS value: ${GLUECODE_LIB_PATH_NAME_DBUS}")
    if (MSVC)
        SET(GLUECODE_SO_DBUS ${GLUECODE_LIB_PATH_NAME_DBUS})
    else()
        # Strip the trailing .0.0.0 from the path as otherwise libCommonAPI won't load the .so
        string(LENGTH ${GLUECODE_LIB_PATH_NAME_DBUS} GLUECODE_LIB_PATH_NAME_DBUS_LENGTH)
        math(EXPR GLUECODE_LIB_PATH_NAME_DBUS_LENGTH_ADJUSTED '${GLUECODE_LIB_PATH_NAME_DBUS_LENGTH}-6')
        string(SUBSTRING ${GLUECODE_LIB_PATH_NAME_DBUS}
            0 ${GLUECODE_LIB_PATH_NAME_DBUS_LENGTH_ADJUSTED}
            GLUECODE_LIB_PATH_NAME_DBUS_LENGTH_CLEAN
        )
        # GLUECODE_SO_DBUS is used in commonapi4dbus.ini.in file
        SET(GLUECODE_SO_DBUS ${GLUECODE_LIB_PATH_NAME_DBUS_LENGTH_CLEAN})
    endif()
endif()

# libSomeIPGlue
if(NOT "${CMAKE_GLUECODE_SOMEIP_NAME}" STREQUAL "na")
    get_property(GLUECODE_LIB_PATH_NAME_SOMEIP TARGET ${CMAKE_GLUECODE_SOMEIP_NAME} PROPERTY IMPORTED_LOCATION_${UPPER_CMAKE_BUILD_TYPE})
    message("GLUECODE_LIB_PATH_NAME_SOMEIP value: ${GLUECODE_LIB_PATH_NAME_SOMEIP}")
    if (MSVC)
        SET(GLUECODE_SO_SOMEIP ${GLUECODE_LIB_PATH_NAME_SOMEIP})
    else()
        # Strip the trailing .0.0.0 from the path as otherwise libCommonAPI won't load the .so
        string(LENGTH ${GLUECODE_LIB_PATH_NAME_SOMEIP} GLUECODE_LIB_PATH_NAME_SOMEIP_LENGTH)
        math(EXPR GLUECODE_LIB_PATH_NAME_SOMEIP_LENGTH_ADJUSTED '${GLUECODE_LIB_PATH_NAME_SOMEIP_LENGTH}-6')
        string(SUBSTRING ${GLUECODE_LIB_PATH_NAME_SOMEIP}
            0 ${GLUECODE_LIB_PATH_NAME_SOMEIP_LENGTH_ADJUSTED}
            GLUECODE_LIB_PATH_NAME_SOMEIP_LENGTH_CLEAN
        )
        # GLUECODE_SO_SOMEIP is used in commonapi4dbus.ini.in file
        SET(GLUECODE_SO_SOMEIP ${GLUECODE_LIB_PATH_NAME_SOMEIP_LENGTH_CLEAN})
    endif()
endif()

##############################################################################
# configure configuration files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/conf/commonapi4dbus.ini.in
  "${PROJECT_BINARY_DIR}/commonapi4dbus.ini" @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/conf/commonapi4someip.ini.in
  "${PROJECT_BINARY_DIR}/commonapi4someip.ini" @ONLY)

##############################################################################
# google test

# check for set environment variable
if(DEFINED ENV{GTEST_ROOT})
    message("GTEST_ROOT is set. GTEST_ROOT = $ENV{GTEST_ROOT}")
else()
    message("GTEST_ROOT is not defined. For building the tests environment variable
             GTEST_ROOT have to be defined. Tests can not be built.")
    # early exit
    return()  # test can not be build -> make check is not available
endif()

# build google test as static library (always) -> therefore deactivate BUILD_SHARED_LIBS in case it is active
set(BUILD_SHARED_LIBS_AUTOMATIC_OFF 0)
if ("${BUILD_SHARED_LIBS}" STREQUAL "ON")
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_SHARED_LIBS_AUTOMATIC_OFF 1)
endif()

# ...but visual studio needs a shared CRT for gtest, otherwise the linker won't work correctly with the testcases.
if (MSVC)
    set( gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" )
endif()

add_subdirectory($ENV{GTEST_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/gtest EXCLUDE_FROM_ALL)
if ("${BUILD_SHARED_LIBS_AUTOMATIC_OFF}" STREQUAL "1")
    set(BUILD_SHARED_LIBS ON)
    set(BUILD_SHARED_LIBS_AUTOMATIC_OFF 0)
endif()

##############################################################################
# verification tests build section

if (MSVC)
# Visual C++ is not always sure whether he is really C++
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCOMMONAPI_INTERNAL_COMPILATION /EHsc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOMMONAPI_INTERNAL_COMPILATION")
set(USE_RT "")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -Wformat -Wformat-security -Wconversion -fexceptions -fstrict-aliasing -fstack-protector -fasynchronous-unwind-tables -fno-omit-frame-pointer -D_GLIBCXX_USE_NANOSLEEP")
set(USE_RT "rt")
endif()

message("Compiler options: ${CMAKE_CXX_FLAGS}") 

include_directories(
    src
    src-gen
    ${COMMONAPI_INCLUDE_DIRS}
    ${gtest_SOURCE_DIR}/include
)

enable_testing()

set(CMAKE_CTEST_COMMAND ctest -V)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

SET(TESTS_BAT "OFF" CACHE BOOL
    "Controls whether only tests for build acceptance should be build and run via 'make check'")

SET(TESTS_EXTRA "OFF" CACHE BOOL
    "Controls whether extra tests should be build or not")

if (MSVC)
set(TEST_LINK_LIBRARIES CommonAPI gtest ws2_32)
else()
set(TEST_LINK_LIBRARIES CommonAPI gtest)
endif()

if (NOT ${TESTS_BAT})
    add_executable(RTLoadingRuntime src/RTLoadingRuntime.cpp)
    target_link_libraries(RTLoadingRuntime ${TEST_LINK_LIBRARIES})

    add_executable(RTBuildProxiesAndStubs
        src/RTBuildProxiesAndStubs.cpp
        src-gen/v1/commonapi/runtime/TestInterfaceStubDefault.cpp)
    target_link_libraries(RTBuildProxiesAndStubs ${TEST_LINK_LIBRARIES})
endif()

add_executable(CMMethodCalls
    src/CMMethodCalls.cpp
    src/stub/CMMethodCallsStub.cpp
    src-gen/v1/commonapi/communication/TestInterfaceStubDefault.cpp)
target_link_libraries(CMMethodCalls ${TEST_LINK_LIBRARIES})

add_executable(CMAttributes
    src/CMAttributes.cpp
    src/stub/CMAttributesStub.cpp
    src-gen/v1/commonapi/communication/TestInterfaceStubDefault.cpp)
target_link_libraries(CMAttributes ${TEST_LINK_LIBRARIES})

add_executable(CMAttributeSubscription
    src/CMAttributeSubscription.cpp
    src-gen/v1/commonapi/communication/TestInterfaceStubDefault.cpp
    src-gen/v1/commonapi/communication/DaemonStubDefault.cpp)
target_link_libraries(CMAttributeSubscription ${TEST_LINK_LIBRARIES})

add_executable(CMBroadcasts
    src/CMBroadcasts.cpp
    src/stub/CMBroadcastsStub.cpp
    src-gen/v1/commonapi/communication/TestInterfaceStubDefault.cpp
    src-gen/v1/commonapi/communication/DaemonStubDefault.cpp)
target_link_libraries(CMBroadcasts ${TEST_LINK_LIBRARIES})

if (NOT ${TESTS_BAT})
    add_executable(AFSelective
        src/AFSelective.cpp
        src/stub/AFSelectiveStub.cpp
        src-gen/v1/commonapi/advanced/bselective/TestInterfaceStubDefault.cpp)
    target_link_libraries(AFSelective ${TEST_LINK_LIBRARIES})

    add_executable(AFManaged
        src/AFManaged.cpp
        src/stub/AFManagedStub.cpp
        src/stub/DeviceStubImpl.cpp
        src/stub/SpecialDeviceStubImpl.cpp
        src-gen/v1/commonapi/advanced/managed/DeviceStubDefault.cpp
        src-gen/v1/commonapi/advanced/managed/SpecialDeviceStubDefault.cpp
        src-gen/v1/commonapi/advanced/managed/ManagerStubDefault.cpp)
    target_link_libraries(AFManaged ${TEST_LINK_LIBRARIES})

    add_executable(AFPolymorph
        src/AFPolymorph.cpp
        src/stub/AFPolymorphStub.cpp
        src-gen/v1/commonapi/advanced/polymorph/TestInterfaceStubDefault.cpp
        src-gen/v1/commonapi/advanced/polymorph/TestInterface.cpp)
    target_link_libraries(AFPolymorph ${TEST_LINK_LIBRARIES})

    add_executable(DTPrimitive
        src/DTPrimitive.cpp
        src/stub/DTPrimitiveStub.cpp
        src-gen/v1/commonapi/datatypes/primitive/TestInterfaceStubDefault.cpp)
    target_link_libraries(DTPrimitive ${TEST_LINK_LIBRARIES})

    add_executable(DTAdvanced
        src/DTAdvanced.cpp
        src/stub/DTAdvancedStub.cpp
        src-gen/v1/commonapi/datatypes/advanced/TestInterfaceStubDefault.cpp)
    target_link_libraries(DTAdvanced ${TEST_LINK_LIBRARIES})

    add_executable(DTDerived
        src/DTDerived.cpp
        src/stub/DTDerivedStub.cpp
        src-gen/v1/commonapi/datatypes/derived/TestInterfaceStubDefault.cpp
        src-gen/v1/commonapi/datatypes/derived/TestInterface.cpp)
    target_link_libraries(DTDerived ${TEST_LINK_LIBRARIES})

    add_executable(DTCombined
        src/DTCombined.cpp
        src/stub/DTCombinedStub.cpp
        src-gen/v1/commonapi/datatypes/combined/TestInterfaceStubDefault.cpp)
    target_link_libraries(DTCombined ${TEST_LINK_LIBRARIES})

    add_executable(THMainLoopIntegration
        src/THMainLoopIntegration.cpp
        src/stub/THMainLoopIntegrationStub.cpp
        src-gen/v1/commonapi/threading/TestInterfaceStubDefault.cpp)
    target_link_libraries(THMainLoopIntegration ${TEST_LINK_LIBRARIES})

    add_executable(THMainLoopIndependence
        src/THMainLoopIndependence.cpp
        src-gen/v1/commonapi/threading/TestInterfaceStubDefault.cpp)
    target_link_libraries(THMainLoopIndependence ${TEST_LINK_LIBRARIES})

    add_executable(THMainLoopTwoThreads
        src/THMainLoopTwoThreads.cpp
        src-gen/v1/commonapi/threading/TestInterfaceStubDefault.cpp)
    target_link_libraries(THMainLoopTwoThreads ${TEST_LINK_LIBRARIES})

    if(${TESTS_EXTRA})
        add_executable(PFPrimitive
            src/PFPrimitive.cpp
            src/stub/PFPrimitiveStub.cpp
            src/utils/StopWatch.cpp
            src-gen/v1/commonapi/performance/primitive/TestInterfaceStubDefault.cpp)
        target_link_libraries(PFPrimitive ${TEST_LINK_LIBRARIES} ${USE_RT})

        add_executable(PFComplex
            src/PFComplex.cpp
            src/stub/PFComplexStub.cpp
            src/utils/StopWatch.cpp
            src-gen/v1/commonapi/performance/complex/TestInterfaceStubDefault.cpp)
        target_link_libraries(PFComplex ${TEST_LINK_LIBRARIES} ${USE_RT})

        add_executable(StabilitySP
            src/StabilitySP.cpp
            src/stub/StabilitySPStub.cpp
            src-gen/v1/commonapi/stability/sp/TestInterfaceStubDefault.cpp)
        target_link_libraries(StabilitySP ${TEST_LINK_LIBRARIES})
    endif()
endif()

if (NOT ${TESTS_BAT})
    add_dependencies(RTLoadingRuntime gtest)
    add_dependencies(RTBuildProxiesAndStubs gtest)
endif()
add_dependencies(CMMethodCalls gtest)
add_dependencies(CMAttributes gtest)
add_dependencies(CMAttributeSubscription gtest)
add_dependencies(CMBroadcasts gtest)
if (NOT ${TESTS_BAT})
    add_dependencies(AFSelective gtest)
    add_dependencies(AFManaged gtest)
    add_dependencies(AFPolymorph gtest)
    add_dependencies(DTPrimitive gtest)
    add_dependencies(DTAdvanced gtest)
    add_dependencies(DTDerived gtest)
    add_dependencies(DTCombined gtest)
    add_dependencies(THMainLoopIntegration gtest)
    add_dependencies(THMainLoopIndependence gtest)
    add_dependencies(THMainLoopTwoThreads gtest)
    if(${TESTS_EXTRA})
        add_dependencies(PFPrimitive gtest)
        add_dependencies(PFComplex gtest)
        add_dependencies(StabilitySP gtest)
    endif()
endif()

if (NOT ${TESTS_BAT})
    add_dependencies(check RTLoadingRuntime)
    add_dependencies(check RTBuildProxiesAndStubs)
endif()
add_dependencies(check CMMethodCalls)
add_dependencies(check CMAttributes)
add_dependencies(check CMAttributeSubscription)
add_dependencies(check CMBroadcasts)
if (NOT ${TESTS_BAT})
    add_dependencies(check AFSelective)
    add_dependencies(check AFManaged)
    add_dependencies(check AFPolymorph)
    add_dependencies(check DTPrimitive)
    add_dependencies(check DTAdvanced)
    add_dependencies(check DTDerived)
    add_dependencies(check DTCombined)
    add_dependencies(check THMainLoopIntegration)
    add_dependencies(check THMainLoopIndependence)
    add_dependencies(check THMainLoopTwoThreads)
    if(${TESTS_EXTRA})
        add_dependencies(check PFPrimitive)
        add_dependencies(check PFComplex)
        add_dependencies(check StabilitySP)
    endif()
endif()

# Tests for DBus
if(NOT "${CMAKE_GLUECODE_DBUS_NAME}" STREQUAL "na")
    # Construct the environment for the tests
    set(DBUS_COMMONAPI_CONFIG "COMMONAPI_CONFIG=commonapi4dbus.ini")
    set(DBUS_TEST_ENVIRONMENT
        ${DBUS_COMMONAPI_CONFIG}
    )

    if (NOT ${TESTS_BAT})
        add_test(NAME RTLoadingRuntimeDBus COMMAND RTLoadingRuntime)
        set_property(TEST RTLoadingRuntimeDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME RTBuildProxiesAndStubsDBus COMMAND RTBuildProxiesAndStubs)
        set_property(TEST RTBuildProxiesAndStubsDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
    endif()
    add_test(NAME CMMethodCallsDBus COMMAND CMMethodCalls)
    set_property(TEST CMMethodCallsDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
    add_test(NAME CMAttributesDBus COMMAND CMAttributes)
    set_property(TEST CMAttributesDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
    add_test(NAME CMAttributeSubscriptionDBus COMMAND CMAttributeSubscription)
    set_property(TEST CMAttributeSubscriptionDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
    add_test(NAME CMBroadcastsDBus COMMAND CMBroadcasts)
    set_property(TEST CMBroadcastsDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
    if (NOT ${TESTS_BAT})
        add_test(NAME AFSelectiveDBus COMMAND AFSelective)
        set_property(TEST AFSelectiveDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME AFManagedDBus COMMAND AFManaged)
        set_property(TEST AFManagedDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME AFPolymorphDBus COMMAND AFPolymorph)
        set_property(TEST AFPolymorphDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME DTPrimitiveDBus COMMAND DTPrimitive)
        set_property(TEST DTPrimitiveDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME DTAdvancedDBus COMMAND DTAdvanced)
        set_property(TEST DTAdvancedDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME DTDerivedDBus COMMAND DTDerived)
        set_property(TEST DTDerivedDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME DTCombinedDBus COMMAND DTCombined)
        set_property(TEST DTCombinedDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME THMainLoopIntegrationDBus COMMAND THMainLoopIntegration)
        set_property(TEST THMainLoopIntegrationDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME THMainLoopIndependenceDBus COMMAND THMainLoopIndependence)
        set_property(TEST THMainLoopIndependenceDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        add_test(NAME THMainLoopTwoThreadsDBus COMMAND THMainLoopTwoThreads)
        set_property(TEST THMainLoopTwoThreadsDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        if(${TESTS_EXTRA})
            add_test(NAME PFPrimitiveDBus COMMAND PFPrimitive)
            set_property(TEST PFPrimitiveDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
            add_test(NAME PFComplexDBus COMMAND PFComplex)
            set_property(TEST PFComplexDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
            add_test(NAME StabilitySPDBus COMMAND StabilitySP)
            set_property(TEST StabilitySPDBus APPEND PROPERTY ENVIRONMENT ${DBUS_TEST_ENVIRONMENT})
        endif()
    endif()
endif()

# Tests for SomeIP
if(NOT "${CMAKE_GLUECODE_SOMEIP_NAME}" STREQUAL "na")
    # Construct the environment for the tests
    set(SOMEIP_COMMONAPI_CONFIG "COMMONAPI_CONFIG=commonapi4someip.ini")
    if ("${SOMEIP_VSOMEIP_JSON_FILE}" STREQUAL "")
        set(SOMEIP_VSOMEIP_JSON_FILE "VSOMEIP_CONFIGURATION=../../../ascgit017.CommonAPI-SomeIP-Tools/org.genivi.commonapi.someip.verification/vsomeip.json")
    endif()
    set(SOMEIP_TEST_ENVIRONMENT
        ${SOMEIP_COMMONAPI_CONFIG}
        ${SOMEIP_VSOMEIP_JSON_FILE}
    )

    if (NOT ${TESTS_BAT})
        add_test(NAME RTLoadingRuntimeSomeIP COMMAND RTLoadingRuntime)
        set_property(TEST RTLoadingRuntimeSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME RTBuildProxiesAndStubsSomeIP COMMAND RTBuildProxiesAndStubs)
        set_property(TEST RTBuildProxiesAndStubsSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
    endif()
    add_test(NAME CMMethodCallsSomeIP COMMAND CMMethodCalls)
    set_property(TEST CMMethodCallsSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
    add_test(NAME CMAttributesSomeIP COMMAND CMAttributes)
    set_property(TEST CMAttributesSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
    add_test(NAME CMAttributeSubscriptionSomeIP COMMAND CMAttributeSubscription)
    set_property(TEST CMAttributeSubscriptionSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
    add_test(NAME CMBroadcastsSomeIP COMMAND CMBroadcasts)
    set_property(TEST CMBroadcastsSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
    if (NOT ${TESTS_BAT})
        add_test(NAME AFSelectiveSomeIP COMMAND AFSelective)
        set_property(TEST AFSelectiveSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME AFManagedSomeIP COMMAND AFManaged)
        set_property(TEST AFManagedSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME AFPolymorphSomeIP COMMAND AFPolymorph)
        set_property(TEST AFPolymorphSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME DTPrimitiveSomeIP COMMAND DTPrimitive)
        set_property(TEST DTPrimitiveSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME DTAdvancedSomeIP COMMAND DTAdvanced)
        set_property(TEST DTAdvancedSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME DTDerivedSomeIP COMMAND DTDerived)
        set_property(TEST DTDerivedSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME DTCombinedSomeIP COMMAND DTCombined)
        set_property(TEST DTCombinedSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME THMainLoopIntegrationSomeIP COMMAND THMainLoopIntegration)
        set_property(TEST THMainLoopIntegrationSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME THMainLoopIndependenceSomeIP COMMAND THMainLoopIndependence)
        set_property(TEST THMainLoopIndependenceSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        add_test(NAME THMainLoopTwoThreadsSomeIP COMMAND THMainLoopTwoThreads)
        set_property(TEST THMainLoopTwoThreadsSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        if(${TESTS_EXTRA})
            add_test(NAME PFPrimitiveSomeIP COMMAND PFPrimitive)
            set_property(TEST PFPrimitiveSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
            add_test(NAME PFComplexSomeIP COMMAND PFComplex)
            set_property(TEST PFComplexSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
            add_test(NAME StabilitySPSomeIP COMMAND StabilitySP)
            set_property(TEST StabilitySPSomeIP APPEND PROPERTY ENVIRONMENT ${SOMEIP_TEST_ENVIRONMENT})
        endif()
    endif()
endif()

##############################################################################
# maintainer-clean
add_custom_target(maintainer-clean COMMAND rm -rf *)

