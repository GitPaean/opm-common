# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.31.6)

# Reject any attempt to use a toolchain file. We must not use one because
# we could be downloading it here. If the CMAKE_TOOLCHAIN_FILE environment
# variable is set, the cache variable will have been initialized from it.
unset(CMAKE_TOOLCHAIN_FILE CACHE)
unset(ENV{CMAKE_TOOLCHAIN_FILE})

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(cjson-populate NONE)



include(ExternalProject)
ExternalProject_Add(cjson-populate
                     "UPDATE_DISCONNECTED" "False" "DOWNLOAD_EXTRACT_TIMESTAMP" "ON" "EXTERNALPROJECT_INTERNAL_ARGUMENT_SEPARATOR" "URL" "https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.16.tar.gz" "URL_HASH" "SHA512=3a894de03c33d89f1e7ee572418d5483c844d38e1e64aa4f6297ddaa01f4111f07601f8d26617b424b5af15d469e3955dae075d9f30b5c25e16ec348fdb06e6f"
                    SOURCE_DIR          "/home/runner/work/opm-common/opm-common/build_test/_deps/cjson-src"
                    BINARY_DIR          "/home/runner/work/opm-common/opm-common/build_test/_deps/cjson-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


