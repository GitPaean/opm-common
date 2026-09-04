# -*- mode: cmake; tab-width: 2; indent-tabs-mode: nil; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 expandtab softtabstop=2 nowrap:

find_package(Boost REQUIRED)
find_package(cJSON)
find_package(fmt)
find_package(QuadMath)

if(TARGET opmcommon)
  get_property(opm-common_EMBEDDED_PYTHON TARGET opmcommon PROPERTY EMBEDDED_PYTHON)
  get_property(opm-common_COMPILE_DEFINITIONS TARGET opmcommon PROPERTY INTERFACE_COMPILE_DEFINITIONS)
  get_property(opm-common_LIBS TARGET opmcommon PROPERTY INTERFACE_LINK_LIBRARIES)

  if(opm-common_EMBEDDED_PYTHON)
    find_package(Python3 COMPONENTS Development.Embed REQUIRED)
  endif()

  if(opm-common_LIBS MATCHES dunecommon)
    find_package(dune-common REQUIRED)
  endif()

  if(opm-common_LIBS MATCHES OpenMP::OpenMP)
    find_package(OpenMP REQUIRED)
  endif()

  # Present when opm-common was built with OPM_LINK_MPI_DIRECTLY (UseMPI.cmake):
  # the imported target is in the exported link interface, so a consumer
  # that has not looked for MPI itself would otherwise fail at generation.
  # The C component: a consumer of this package has enabled C, as
  # OpenMP::OpenMP_C in the same exported interface already requires
  # (UseOpenMP.cmake links it publicly whenever OpenMP is used).
  if(opm-common_LIBS MATCHES "MPI::MPI_C")
    find_package(MPI REQUIRED COMPONENTS C)
  endif()
endif()
