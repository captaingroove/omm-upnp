cmake_minimum_required(VERSION 2.6.2)

project(omm)

# version number and commit identifier
set(OMM_VERSION_MAJOR 0)
set(OMM_VERSION_MINOR 1)
set(OMM_VERSION_PATCH 0)
set(OMM_VERSION "${OMM_VERSION_MAJOR}.${OMM_VERSION_MINOR}.${OMM_VERSION_PATCH}")
add_definitions(-D__OMM_VERSION_STRING__="${OMM_VERSION}")
file(READ "COMMIT" OMM_COMMIT_FILE)
string(STRIP ${OMM_COMMIT_FILE} OMM_COMMIT)
add_definitions(-D__OMM_COMMIT_STRING__="${OMM_COMMIT}")
message(STATUS "Building OMM version " ${OMM_VERSION} " (" ${OMM_COMMIT} ")")

# determine build and target system and set variables for cross compiling
if(CMAKE_CROSSCOMPILING)
message(STATUS "Host system: " ${CMAKE_HOST_SYSTEM_NAME})
message(STATUS "Building for target system: " ${CMAKE_SYSTEM_NAME})
else(CMAKE_CROSSCOMPILING)
message(STATUS "Building native for system: " ${CMAKE_SYSTEM_NAME})
endif(CMAKE_CROSSCOMPILING)

# override BUILD_TARGET if not specified
if(LINUX OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(LINUX 1)
set(BUILD_TARGET "Linux")
add_definitions(-D__LINUX__)
elseif(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(DARWIN 1)
set(MACOSX 1)
add_definitions(-D__DARWIN__)
add_definitions(-D__MACOSX__)
# set install_name for libraries on Darwin
set(CMAKE_INSTALL_NAME_DIR
${CMAKE_INSTALL_PREFIX}/lib
)
elseif(WINDOWS OR ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(WINDOWS 1)
add_definitions(-D__WINDOWS__)
set(BUILD_TARGET "Windows")
elseif(NOT BUILD_TARGET)
set(BUILD_TARGET "Native")
endif(LINUX OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# iphone or iphone-simulator
if("${BUILD_TARGET}" MATCHES "iphone")
set(DARWIN 1)
set(IPHONE 1)
add_definitions(-D__DARWIN__)
add_definitions(-D__IPHONE__)
#set(CMAKE_AR "arm-apple-darwin11-ar")
#set(CMAKE_RANLIB "arm-apple-darwin11-ranlib")
endif("${BUILD_TARGET}" MATCHES "iphone")

# android
if("${BUILD_TARGET}" MATCHES "android")
set(ANDROID 1)
add_definitions(-D__ANDROID__)
endif("${BUILD_TARGET}" MATCHES "android")

message(STATUS "Build target is: " ${BUILD_TARGET})

# iphone only
if("${BUILD_TARGET}" STREQUAL iphone)
# use hardware floating point ops on simulator and softemu on iphone device
add_definitions(-msoft-float)
endif("${BUILD_TARGET}" STREQUAL iphone)

if(${CMAKE_VERSION} VERSION_GREATER "2.8.11")
add_compile_options(-Wno-unused-parameter) # introduced in cmake 2.8.12
endif(${CMAKE_VERSION} VERSION_GREATER "2.8.11")

get_property(BUILD_SHARED_LIBS
GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
set(CMAKE_MODULE_PATH
${CMAKE_CURRENT_SOURCE_DIR}/cmake
${CMAKE_MODULE_PATH}
)

# check basic dependency Poco
find_package(POCO REQUIRED)

# set OMM lib and plugin includes
set(libdir "${CMAKE_CURRENT_SOURCE_DIR}/src/lib")
if(IS_DIRECTORY ${libdir})
# omm libs are in the source tree so build them first and use internal headers
message(STATUS "Using internal OMM libraries")
include_directories(src/include)
else(IS_DIRECTORY ${libdir})
message(STATUS "Search for external OMM libraries ...")
find_package(OMM REQUIRED)
endif(IS_DIRECTORY ${libdir})

set(plugindir "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin")
if(IS_DIRECTORY ${plugindir})
# omm plugins are in the source tree so build them first and use internal headers
message(STATUS "Using internal OMM plugins")
include_directories(${plugindir}/include)
else(IS_DIRECTORY ${plugindir})
message(STATUS "Search for external OMM plugins ...")
find_package(OMMPLUGIN)
endif(IS_DIRECTORY ${plugindir})

# set Poco includes. Must be after OMM headers because Poco and OMM could be
# in staging dir and internal OMM headers are always prefered.
include_directories(${POCO_INCLUDE_DIRS})

# build what is available in the source tree (libs, plugins, apps, tests)
add_subdirectory(src)
add_subdirectory(bindings)

# CPack related for creating binary distribution packages
include(ConfigurePackager)
include(CPack)
