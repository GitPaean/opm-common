# - Open Porous Media Initiative shared infrastructure config mode
#
# Defines the following variables:
#  opm-common_FOUND        - true
#  opm-common_VERSION      - version of the opm-common library found, e.g. 0.2
#  opm-common_DEFINITIONS  - defines to be made on the command line
#  opm-common_INCLUDE_DIRS - header directories with which to compile
#  opm-common_LINKER_FLAGS - flags that must be passed to the linker
#  opm-common_LIBRARIES    - names of the libraries with which to link
#  opm-common_LIBRARY_DIRS - directories in which the libraries are situated
#
# You should put lines like this in your CMakeLists.txt
#  set (opm-common_DIR "${PROJECT_BINARY_DIR}/../opm-common" CACHE LOCATION "Build tree of opm-common")
#  find_package (opm-common)
#  configure_vars (
#    FILE  CXX  "${PROJECT_BINARY_DIR}/config.h"
#    WRITE ${opm-common_CONFIG_VARS}
#    )

# <http://www.vtk.org/Wiki/CMake/Tutorials/How_to_create_a_ProjectConfig.cmake_file>

# Prevent multiple inclusions
if(NOT opm-common_FOUND)
  # propagate these properties from one build system to the other
  set (opm-common_PREFIX "/usr/local")
  set (opm-common_VERSION "2026.04")
  set (opm-common_DEFINITIONS "")
  set (opm-common_INCLUDE_DIRS "/usr/local/include;/usr/include")
  set (opm-common_LIBRARY_DIRS "" "/usr/local/lib")
  set (opm-common_LINKER_FLAGS "")
  set (opm-common_CONFIG_VARS "HAVE_OPENMP;HAVE_TYPE_TRAITS;HAVE_FINAL;HAVE_ECL_INPUT;HAVE_CXA_DEMANGLE;HAVE_FNMATCH_H;HAVE_DUNE_COMMON")

  # libraries come from the build tree where this file was generated
  set (opm-common_LIBRARY "opmcommon")
  set (opm-common_LIBRARIES ${opm-common_LIBRARY} "")

  set (HAVE_OPM_COMMON 1)
  mark_as_advanced (opm-common_LIBRARY)

  # not all projects have targets; conditionally add this part
  if (NOT "opmcommon" STREQUAL "")
    # add the library as a target, so that other things in the project including
    # this file may depend on it and get rebuild if this library changes.
    if(NOT TARGET opmcommon)
      get_filename_component(_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
      include("${_dir}/opm-common-targets.cmake")
    endif()
  endif (NOT "opmcommon" STREQUAL "")

  # if this file is not processed using the OPM CMake system but
  # simply by a call to find_package(module) then the CMAKE_MODULE_PATH
  # might not include the location of the OPM cmake module yet.
  # Hence we search for opm-common using config mode to set it up.
  if(NOT TARGET opmcommon)
    # This needed to find the path to the CMake modules
    find_package(opm-common CONFIG)
  endif()

  set(OPM_MACROS_ROOT /usr/local/share/opm)
  list(APPEND CMAKE_MODULE_PATH ${OPM_MACROS_ROOT}/cmake/Modules)
  include(OpmPackage)

  # This is required to include OpmPackage /opm-common-prereq.cmake
  list(PREPEND CMAKE_MODULE_PATH /usr/local/share/opm/cmake/Modules)

  include(opm-common-prereqs)

  # remove the temporarily added path again to not pollute downstream modules
  list(POP_FRONT CMAKE_MODULE_PATH)
endif()

# this is the contents of config.h as far as our probes can tell:
if (DEFINED HAVE_OPENMP AND NOT (("${HAVE_OPENMP}" STREQUAL "") OR ("${HAVE_OPENMP}" STREQUAL "1") OR ("${HAVE_OPENMP}" STREQUAL "TRUE") OR ("${HAVE_OPENMP}" STREQUAL "ON")))
	message (WARNING "Incompatible value \"${HAVE_OPENMP}\" of variable \"HAVE_OPENMP\"")
endif ()
set (HAVE_OPENMP)
if (DEFINED HAVE_TYPE_TRAITS AND NOT (("${HAVE_TYPE_TRAITS}" STREQUAL "") OR ("${HAVE_TYPE_TRAITS}" STREQUAL "1") OR ("${HAVE_TYPE_TRAITS}" STREQUAL "TRUE") OR ("${HAVE_TYPE_TRAITS}" STREQUAL "ON")))
	message (WARNING "Incompatible value \"${HAVE_TYPE_TRAITS}\" of variable \"HAVE_TYPE_TRAITS\"")
endif ()
set (HAVE_TYPE_TRAITS)
if (DEFINED HAVE_FINAL AND NOT (("${HAVE_FINAL}" STREQUAL "") OR ("${HAVE_FINAL}" STREQUAL "1") OR ("${HAVE_FINAL}" STREQUAL "TRUE") OR ("${HAVE_FINAL}" STREQUAL "ON")))
	message (WARNING "Incompatible value \"${HAVE_FINAL}\" of variable \"HAVE_FINAL\"")
endif ()
set (HAVE_FINAL)
if (DEFINED HAVE_ECL_INPUT AND NOT (("${HAVE_ECL_INPUT}" STREQUAL "") OR ("${HAVE_ECL_INPUT}" STREQUAL "1") OR ("${HAVE_ECL_INPUT}" STREQUAL "TRUE") OR ("${HAVE_ECL_INPUT}" STREQUAL "ON")))
	message (WARNING "Incompatible value \"${HAVE_ECL_INPUT}\" of variable \"HAVE_ECL_INPUT\"")
endif ()
set (HAVE_ECL_INPUT)
if (DEFINED HAVE_CXA_DEMANGLE AND NOT (("${HAVE_CXA_DEMANGLE}" STREQUAL "") OR ("${HAVE_CXA_DEMANGLE}" STREQUAL "1") OR ("${HAVE_CXA_DEMANGLE}" STREQUAL "TRUE") OR ("${HAVE_CXA_DEMANGLE}" STREQUAL "ON")))
	message (WARNING "Incompatible value \"${HAVE_CXA_DEMANGLE}\" of variable \"HAVE_CXA_DEMANGLE\"")
endif ()
set (HAVE_CXA_DEMANGLE)
if (DEFINED HAVE_FNMATCH_H AND NOT (("${HAVE_FNMATCH_H}" STREQUAL "") OR ("${HAVE_FNMATCH_H}" STREQUAL "1") OR ("${HAVE_FNMATCH_H}" STREQUAL "TRUE") OR ("${HAVE_FNMATCH_H}" STREQUAL "ON")))
	message (WARNING "Incompatible value \"${HAVE_FNMATCH_H}\" of variable \"HAVE_FNMATCH_H\"")
endif ()
set (HAVE_FNMATCH_H "1")
if (DEFINED HAVE_DUNE_COMMON AND NOT (("${HAVE_DUNE_COMMON}" STREQUAL "") OR ("${HAVE_DUNE_COMMON}" STREQUAL "1") OR ("${HAVE_DUNE_COMMON}" STREQUAL "TRUE") OR ("${HAVE_DUNE_COMMON}" STREQUAL "ON")))
	message (WARNING "Incompatible value \"${HAVE_DUNE_COMMON}\" of variable \"HAVE_DUNE_COMMON\"")
endif ()
set (HAVE_DUNE_COMMON)
