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

project(fmt-populate NONE)



include(ExternalProject)
ExternalProject_Add(fmt-populate
                     "UPDATE_DISCONNECTED" "False" "DOWNLOAD_EXTRACT_TIMESTAMP" "ON" "EXTERNALPROJECT_INTERNAL_ARGUMENT_SEPARATOR" "URL" "https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz" "URL_HASH" "SHA512=288c349baac5f96f527d5b1bed0fa5f031aa509b4526560c684281388e91909a280c3262a2474d963b5d1bf7064b1c9930c6677fe54a0d8f86982d063296a54c"
                    SOURCE_DIR          "/home/runner/work/opm-common/opm-common/build_test/_deps/fmt-src"
                    BINARY_DIR          "/home/runner/work/opm-common/opm-common/build_test/_deps/fmt-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


