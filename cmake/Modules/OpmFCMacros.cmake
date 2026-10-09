# Copyright 2026 SINTEF Digital, Mathematics & Cybernetics.
#
# - A fallback FCMacros.h for when FortranCInterface cannot produce one
#
# opm/common/utility/numeric/blas_lapack.h #errors unless <FCMacros.h> can be
# found, and the modules that reach it through that header - opm-grid and
# opm-upscaling - have no Fortran sources of their own: they enable the
# language only so that FortranCInterface can detect how the BLAS/LAPACK
# library spells its symbols. Two things leave that detection without a
# result. No Fortran compiler at all is one. A compiler FortranCInterface
# cannot link against the C toolchain is the other (MinGW gfortran on PATH
# beside an MSVC/Ninja toolchain, say): that is reported with a status
# message rather than an error, and the header it leaves behind defines no
# macros, which __has_include() is still happy to find. So test the result,
# FortranCInterface_GLOBAL_FOUND, rather than the compiler:
#
#   macro(<module>_language_hook)
#     if(CMAKE_Fortran_COMPILER)
#       include(FortranCInterface)
#       fortrancinterface_header(${PROJECT_BINARY_DIR}/FCMacros.h MACRO_NAMESPACE FC_)
#     endif()
#     include(OpmFCMacros)
#     if(NOT FortranCInterface_GLOBAL_FOUND)
#       opm_write_fallback_fcmacros(${PROJECT_BINARY_DIR}/FCMacros.h)
#     endif()
#   endmacro()
#
# opm_write_fallback_fcmacros(<path>) writes a header assuming the de-facto
# standard mangling - lowercase with a trailing underscore - used by
# gfortran-built BLAS/LAPACK (OpenBLAS, reference LAPACK) and also exported by
# MKL. It rewrites the file only when the contents differ, so a reconfigure
# does not needlessly invalidate everything that includes it.
include_guard(GLOBAL)

function(opm_write_fallback_fcmacros path)
  set(content
"#ifndef FC_HEADER_INCLUDED
#define FC_HEADER_INCLUDED

/* Fallback Fortran name mangling, written without a working FortranCInterface
   detection: lowercase with trailing underscore (gfortran/OpenBLAS/LAPACK). */
#define FC_GLOBAL(name,NAME) name##_
#define FC_GLOBAL_(name,NAME) name##_

#endif
")
  set(existing "")
  if(EXISTS "${path}")
    file(READ "${path}" existing)
  endif()
  if(NOT existing STREQUAL content)
    message(STATUS "No usable FortranCInterface result - writing fallback FCMacros.h "
                   "assuming lowercase-with-underscore BLAS/LAPACK name mangling "
                   "(gfortran/OpenBLAS/LAPACK/MKL convention). If linking BLAS/LAPACK "
                   "fails with unresolved symbols like 'dgemm_', your library uses a "
                   "different convention; configure with -DCMAKE_Fortran_COMPILER=... "
                   "so the real mangling can be detected.")
    file(WRITE "${path}" "${content}")
  endif()
endfunction()
