cmake_minimum_required(VERSION 3.5)

project(eosforce_contracts)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 6)
set(VERSION_PATCH 0)
#set(VERSION_SUFFIX rc3)

if (VERSION_SUFFIX)
   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX}")
else()
   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
endif()

include(ExternalProject)

find_package(eosforce.cdt)

message(STATUS "Building eosforce.contracts v${VERSION_FULL}")

set(EOSFORCE_CDT_VERSION_MIN "1.5")
set(EOSFORCE_CDT_VERSION_SOFT_MAX "1.5")
#set(EOSFORCE_CDT_VERSION_HARD_MAX "")

### Check the version of eosforce.cdt
set(VERSION_MATCH_ERROR_MSG "")
EOSIO_CHECK_VERSION(VERSION_OUTPUT "${EOSFORCE_CDT_VERSION}"
                                   "${EOSFORCE_CDT_VERSION_MIN}"
                                   "${EOSFORCE_CDT_VERSION_SOFT_MAX}"
                                   "${EOSFORCE_CDT_VERSION_HARD_MAX}"
                                   VERSION_MATCH_ERROR_MSG)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   set(TEST_BUILD_TYPE "Debug")
   set(CMAKE_BUILD_TYPE "Release")
else()
   set(TEST_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

ExternalProject_Add(
   contracts_project
   SOURCE_DIR ${CMAKE_SOURCE_DIR}/contracts
   BINARY_DIR ${CMAKE_BINARY_DIR}/contracts
   CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${EOSFORCE_CDT_ROOT}/lib/cmake/eosforce.cdt/EosioWasmToolchain.cmake
   UPDATE_COMMAND ""
   PATCH_COMMAND ""
   TEST_COMMAND ""
   INSTALL_COMMAND ""
   BUILD_ALWAYS 1
)

