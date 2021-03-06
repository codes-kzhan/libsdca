cmake_minimum_required(VERSION 2.8.9)
project(libsdca)

set(libsdca_MAJOR_VERSION 0)
set(libsdca_MINOR_VERSION 3)
set(libsdca_PATCH_VERSION 1)
set(libsdca_VERSION
  ${libsdca_MAJOR_VERSION}.${libsdca_MINOR_VERSION}.${libsdca_PATCH_VERSION}
  )
add_definitions(-DSDCA_VERSION="${libsdca_VERSION}")

# Use either fast or accurate math
#   TRUE    enables -Ofast (with GCC) and disables SDCA_ACCURATE_MATH
#   FALSE   disables -Ofast and enables SDCA_ACCURATE_MATH
set(USE_FAST_MATH TRUE)

# Whether to use <chrono>
#   TRUE    use std::chrono::high_resolution_clock to measure wall-clock time
#   FALSE   use gettimeofday from <sys/time.h>
set(USE_CHRONO FALSE)

###############################################################################
#
# Build Configuration
#
###############################################################################

#------------------------------------------------------------------------------
# Compiler Flags
#------------------------------------------------------------------------------
# Select Release as the default build type
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Warning flags
set(WARN_FLAGS "-pedantic -Werror -W -Wall -Wextra")
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(WARN_FLAGS "${WARN_FLAGS} -Weverything")
  set(WARN_FLAGS "${WARN_FLAGS} -Wno-c++98-compat")
  set(WARN_FLAGS "${WARN_FLAGS} -Wno-float-equal")
  set(WARN_FLAGS "${WARN_FLAGS} -Wno-documentation")
  set(WARN_FLAGS "${WARN_FLAGS} -Wno-padded")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(WARN_FLAGS "${WARN_FLAGS} -Wparentheses")
  set(WARN_FLAGS "${WARN_FLAGS} -Wshadow")
  set(WARN_FLAGS "${WARN_FLAGS} -Wpointer-arith")
  set(WARN_FLAGS "${WARN_FLAGS} -Wcast-qual")
  set(WARN_FLAGS "${WARN_FLAGS} -Wcast-align")
  set(WARN_FLAGS "${WARN_FLAGS} -Wwrite-strings")
  #set(WARN_FLAGS "${WARN_FLAGS} -Wconversion")
  set(WARN_FLAGS "${WARN_FLAGS} -Wmissing-declarations")
  set(WARN_FLAGS "${WARN_FLAGS} -Wredundant-decls")
  set(WARN_FLAGS "${WARN_FLAGS} -Wstack-protector")
  set(WARN_FLAGS "${WARN_FLAGS} -Wuseless-cast")
  set(WARN_FLAGS "${WARN_FLAGS} -Wdouble-promotion")
  set(WARN_FLAGS "${WARN_FLAGS} -Wvector-operation-performance")
  set(WARN_FLAGS "${WARN_FLAGS} -Wlogical-op")
endif()
# TODO: fix the unit tests and enable this warning again
set(WARN_FLAGS "${WARN_FLAGS} -Wno-conversion")

# Debug flags
set(DEBUG_FLAGS "-fstack-protector-all")

# Release flags
# set(RELEASE_FLAGS "-march=native")
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(RELEASE_FLAGS "-mtune=generic")
  if(${USE_FAST_MATH})
    # Tests pass with -Ofast using gcc-4.9.2, but not clang-7.0.2
    set(RELEASE_FLAGS "${RELEASE_FLAGS} -Ofast")
  endif()
  set(RELEASE_FLAGS "${RELEASE_FLAGS} -funsafe-loop-optimizations")
  set(RELEASE_FLAGS "${RELEASE_FLAGS} -flto -flto-partition=none")
  set(RELEASE_FLAGS "${RELEASE_FLAGS} -fno-fat-lto-objects")
  set(RELEASE_FLAGS "${RELEASE_FLAGS} -fuse-linker-plugin")
endif()

#------------------------------------------------------------------------------
# Intel MKL configuration (not applicable if another BLAS is used)
#------------------------------------------------------------------------------
# Whether to use 64 or 32 bit integers (default: TRUE)
set(USE_ILP64 TRUE)

# Whether to use sequential or multi-threaded Intel MKL layer (default: FALSE)
set(USE_SEQUENTIAL FALSE)

# Whether to add a suffix (e.g. st/mt) for single-/multi-threaded versions
set(USE_THREADING_SUFFIX FALSE)


#------------------------------------------------------------------------------
# Auxiliary Cmake Configuration
#------------------------------------------------------------------------------
# Custom Cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Install prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
    "${CMAKE_SOURCE_DIR}" CACHE PATH "Installation directory." FORCE
    )
endif()

# Avoid a warning due to CMP0042
if(POLICY CMP0042 AND APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

# RPATH settings
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "\\\$ORIGIN")


###############################################################################
#
# Code Dependencies and Third Party Libraries
#
###############################################################################

#------------------------------------------------------------------------------
# Matlab Libraries
#------------------------------------------------------------------------------
# Honor the visibility properties set for targets of type OBJECT
# (visibility is set to 'hidden' for mex files, see FindMatlab.cmake)
if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()
find_package(Matlab COMPONENTS MX_LIBRARY BLAS_LIBRARY IOMP_LIBRARY)

#------------------------------------------------------------------------------
# BLAS Libraries
#------------------------------------------------------------------------------
# BLAS libraries from various vendors (e.g. Atlas or Accelerate)
find_package(BLAS)

# Intel MKL
find_package(MKL)

# Select a BLAS library and set the corresponding compiler/linker flags
include(SelectBLAS)

set(CMAKE_SHARED_LINKER_FLAGS
  "${CMAKE_SHARED_LINKER_FLAGS} ${BLAS_LINKER_FLAGS}"
  )

#------------------------------------------------------------------------------
# Google Test Framework
#------------------------------------------------------------------------------

# At this point, we only need googletest, not googlemock
set(GOOGLETEST_PATH ${CMAKE_SOURCE_DIR}/vendor/googletest/googletest)

#add_subdirectory(${GOOGLETEST_PATH})

find_package(Threads REQUIRED)

include(ExternalProject)
ExternalProject_Add(googletest
  SOURCE_DIR "${GOOGLETEST_PATH}"
  BINARY_DIR "${CMAKE_BINARY_DIR}/googletest-build"
  PREFIX "${CMAKE_BINARY_DIR}/googletest"
  INSTALL_COMMAND   ""
  )

ExternalProject_Get_Property(googletest source_dir binary_dir)
add_library(gtest UNKNOWN IMPORTED)
set_target_properties(gtest PROPERTIES
  IMPORTED_LOCATION ${binary_dir}/libgtest.a
  IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
  INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/include"
)

add_dependencies(gtest googletest)


###############################################################################
#
# SDCA Library
#
###############################################################################

if(NOT ${USE_FAST_MATH})
  # If defined, more computations are performed to increase numerical stability
  add_definitions(-DSDCA_ACCURATE_MATH)
endif()

if(${USE_CHRONO})
  # If defind, std::chrono::high_resolution_clock is used in stopwatch
  add_definitions(-DSDCA_USE_CHRONO)
endif()


# Set SDCA specific flags after 3rd party code is built
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -std=c++11 ${WARN_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE
  "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${RELEASE_FLAGS}")

include_directories(include)

set(libsdca_INCLUDE_PATH
  ${CMAKE_SOURCE_DIR}/include/sdca
  )

set(libsdca_MATH_SOURCES
  ${libsdca_INCLUDE_PATH}/math/blas.h
  ${libsdca_INCLUDE_PATH}/math/functor.h
  ${libsdca_INCLUDE_PATH}/math/lambert.h
  ${libsdca_INCLUDE_PATH}/math/log_exp.h
  )

set(libsdca_PROX_SOURCES
  ${libsdca_INCLUDE_PATH}/prox.h
  ${libsdca_INCLUDE_PATH}/prox/proxdef.h
  ${libsdca_INCLUDE_PATH}/prox/entropy.h
  ${libsdca_INCLUDE_PATH}/prox/entropy_norm.h
  ${libsdca_INCLUDE_PATH}/prox/knapsack_eq.h
  ${libsdca_INCLUDE_PATH}/prox/knapsack_le.h
  ${libsdca_INCLUDE_PATH}/prox/knapsack_le_biased.h
  ${libsdca_INCLUDE_PATH}/prox/topk_cone.h
  ${libsdca_INCLUDE_PATH}/prox/topk_cone_biased.h
  ${libsdca_INCLUDE_PATH}/prox/topk_entropy.h
  ${libsdca_INCLUDE_PATH}/prox/topk_entropy_biased.h
  ${libsdca_INCLUDE_PATH}/prox/topk_simplex.h
  ${libsdca_INCLUDE_PATH}/prox/topk_simplex_biased.h
  ${libsdca_INCLUDE_PATH}/prox/two_entropy.h
  ${libsdca_INCLUDE_PATH}/prox/two_simplex.h
  ${libsdca_INCLUDE_PATH}/prox/two_simplex_sort.h
)

set(libsdca_SOLVER_SOURCES
  ${libsdca_INCLUDE_PATH}/solver.h
  ${libsdca_INCLUDE_PATH}/solver/solverdef.h
  ${libsdca_INCLUDE_PATH}/solver/context.h
  ${libsdca_INCLUDE_PATH}/solver/data/dataset.h
  ${libsdca_INCLUDE_PATH}/solver/data/input.h
  ${libsdca_INCLUDE_PATH}/solver/data/output.h
  ${libsdca_INCLUDE_PATH}/solver/data/scratch.h
  ${libsdca_INCLUDE_PATH}/solver/data.h
  ${libsdca_INCLUDE_PATH}/solver/eval/core.h
  ${libsdca_INCLUDE_PATH}/solver/eval/dual.h
  ${libsdca_INCLUDE_PATH}/solver/eval/primal.h
  ${libsdca_INCLUDE_PATH}/solver/eval/regularizer.h
  ${libsdca_INCLUDE_PATH}/solver/eval/scores.h
  ${libsdca_INCLUDE_PATH}/solver/eval/types.h
  ${libsdca_INCLUDE_PATH}/solver/eval.h
  ${libsdca_INCLUDE_PATH}/solver/objective/l2_entropy.h
  ${libsdca_INCLUDE_PATH}/solver/objective/l2_entropy_topk.h
  ${libsdca_INCLUDE_PATH}/solver/objective/l2_entropy_nn_features.h
  ${libsdca_INCLUDE_PATH}/solver/objective/l2_hinge_topk.h
  ${libsdca_INCLUDE_PATH}/solver/objective/l2_multilabel_entropy.h
  ${libsdca_INCLUDE_PATH}/solver/objective/l2_multilabel_hinge.h
  ${libsdca_INCLUDE_PATH}/solver/objective/l2_topk_hinge.h
  ${libsdca_INCLUDE_PATH}/solver/objective/objective_base.h
  ${libsdca_INCLUDE_PATH}/solver/objective.h
  ${libsdca_INCLUDE_PATH}/solver/reporting.h
  ${libsdca_INCLUDE_PATH}/solver/update.h
  )

set(libsdca_UTILITY_SOURCES
  ${libsdca_INCLUDE_PATH}/utility/logging.h
  ${libsdca_INCLUDE_PATH}/utility/stopwatch.h
  ${libsdca_INCLUDE_PATH}/utility/types.h
  )


#------------------------------------------------------------------------------
# Interfaces
#------------------------------------------------------------------------------
set(SDCA_BINARY_SUFFIX)
if(${USE_THREADING_SUFFIX})
  if(${USE_SEQUENTIAL})
    set(SDCA_BINARY_SUFFIX _st)
  else()
    set(SDCA_BINARY_SUFFIX _mt)
  endif()
endif()

set(SDCA_MATLAB_PREFIX matsdca_)
set(SDCA_MATLAB_INSTALL_DIR matlab)

add_subdirectory(src)


#------------------------------------------------------------------------------
# Unit tests
#------------------------------------------------------------------------------
add_subdirectory(test)

enable_testing()
add_test(test_prox test/testprox)
add_test(test_solver test/testsolver)
