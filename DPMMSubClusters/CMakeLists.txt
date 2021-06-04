if (WIN32)
    set(CMAKE_CUDA_COMPILER "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/bin/nvcc.exe")
    set(CMAKE_CUBLAS_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/lib/x64")
endif (WIN32)

if (UNIX)
    set(CMAKE_CUDA_COMPILER "/usr/local/cuda-11/bin/nvcc")
    set(CMAKE_CUBLAS_DIR "/usr/local/cuda-11.3/lib64")
endif (UNIX)

#set(CMAKE_PROJECT_VERSION 1)
#set(CMAKE_PROJECT_VERSION_MAJOR 0)
#set(CMAKE_PROJECT_VERSION_MINOR 0)
set(PROJECT_NAME "DPMMSubClusters")
set(CMAKE_PROJECT_NAME ${PROJECT_NAME})
cmake_policy(SET CMP0048 NEW)
project(${CMAKE_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX CUDA C)

cmake_minimum_required(VERSION 3.8)

#if(NOT DEFINED ${CMAKE_CUDA_ARCHITECTURES})
#    set(CMAKE_CUDA_ARCHITECTURES 75)
#endif()
#message(STATUS "CUDA architectures set to ${CMAKE_CUDA_ARCHITECTURES}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckCXXCompilerFlag)
include(GNUInstallDirs)

LIST(APPEND CMAKE_PROGRAM_PATH  ${CUDA_TOOLKIT_ROOT_DIR})

set(EIGEN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/eigen-3.3.9)
add_subdirectory("${CMAKE_SOURCE_DIR}/eigen-3.3.9")


if (WIN32)
	set(OpenCV_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/opencv)
	include_directories(${OpenCV_INCLUDE_DIRS})
endif (WIN32)


include_directories(${EIGEN_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/eigen-3.3.9)
include_directories(${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

set(STATS_DIR "${CMAKE_SOURCE_DIR}/stats/include")
include_directories(${STATS_DIR})

set(GCEM_DIR "${CMAKE_SOURCE_DIR}/gcem/include")
include_directories(${GCEM_DIR})

# include_directories(${OpenCV_INCLUDE_DIRS})

set(JSON_DIR "${CMAKE_SOURCE_DIR}/jsoncpp")
include_directories(${JSON_DIR}/include)

set(MITOOLBOX_DIR "${CMAKE_SOURCE_DIR}/MIToolbox")
include_directories(${MITOOLBOX_DIR}/include)

set(CNPY_DIR "${CMAKE_SOURCE_DIR}/cnpy")
include_directories(${CNPY_DIR})

set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib")
include_directories(${ZLIB_INCLUDE_DIR})

set(DISTRIBUTIONS_DIR "${CMAKE_SOURCE_DIR}/distributions")
include_directories(${DISTRIBUTIONS_DIR})

set(DISTRIBUTIONS_UTIL_DIR ${CMAKE_SOURCE_DIR}/distributions_util)
include_directories(${DISTRIBUTIONS_UTIL_DIR})

set(PRIORS_DIR "${CMAKE_SOURCE_DIR}/priors")
include_directories(${PRIORS_DIR})

include_directories(${CMAKE_SOURCE_DIR})

if (UNIX)
    find_package(OpenCV REQUIRED)
endif (UNIX)


add_executable(DPMMSubClusters
  ${CMAKE_SOURCE_DIR}/check_time.cpp
  ${CMAKE_SOURCE_DIR}/check_time.h
  ${CMAKE_SOURCE_DIR}/cudaKernel.cu
  ${CMAKE_SOURCE_DIR}/cudaKernel.cuh
  ${CMAKE_SOURCE_DIR}/cudaKernel_gaussian.cu
  ${CMAKE_SOURCE_DIR}/cudaKernel_gaussian.cuh
  ${CMAKE_SOURCE_DIR}/cudaKernel_multinomial.cu
  ${CMAKE_SOURCE_DIR}/cudaKernel_multinomial.cuh
  ${CMAKE_SOURCE_DIR}/data_generators.cpp
  ${CMAKE_SOURCE_DIR}/data_generators.h
  ${CMAKE_SOURCE_DIR}/distribution_sample.cpp
  ${CMAKE_SOURCE_DIR}/distribution_sample.h
  ${CMAKE_SOURCE_DIR}/dp_parallel_sampling.cpp
  ${CMAKE_SOURCE_DIR}/dp_parallel_sampling.h
  ${CMAKE_SOURCE_DIR}/draw.cpp
  ${CMAKE_SOURCE_DIR}/draw.h
  ${CMAKE_SOURCE_DIR}/ds.h
  ${CMAKE_SOURCE_DIR}/global_params.h
  ${CMAKE_SOURCE_DIR}/global_params.cpp
  ${CMAKE_SOURCE_DIR}/hyperparams.h
  ${CMAKE_SOURCE_DIR}/hyperparams.cpp
  ${CMAKE_SOURCE_DIR}/local_clusters_actions.cpp
  ${CMAKE_SOURCE_DIR}/local_clusters_actions.h
  ${CMAKE_SOURCE_DIR}/main.cpp
  ${CMAKE_SOURCE_DIR}/module_tests.cpp
  ${CMAKE_SOURCE_DIR}/module_tests.h
  ${CMAKE_SOURCE_DIR}/moduleTypes.h
  ${CMAKE_SOURCE_DIR}/shared_actions.cpp
  ${CMAKE_SOURCE_DIR}/shared_actions.h
  ${CMAKE_SOURCE_DIR}/sufficient_statistics.cpp
  ${CMAKE_SOURCE_DIR}/sufficient_statistics.h
  ${CMAKE_SOURCE_DIR}/utils.cpp
  ${CMAKE_SOURCE_DIR}/utils.h
  ${PRIORS_DIR}/multinomial_prior.cpp
  ${PRIORS_DIR}/multinomial_prior.h
  ${PRIORS_DIR}/multinomial_hyper.cpp
  ${PRIORS_DIR}/multinomial_hyper.h
  ${PRIORS_DIR}/multinomial_sufficient_statistics.cpp
  ${PRIORS_DIR}/multinomial_sufficient_statistics.h
  ${PRIORS_DIR}/niw.cpp
  ${PRIORS_DIR}/niw.h
  ${PRIORS_DIR}/niw_hyperparams.cpp
  ${PRIORS_DIR}/niw_hyperparams.h
  ${PRIORS_DIR}/niw_sufficient_statistics.cpp
  ${PRIORS_DIR}/niw_sufficient_statistics.h
  ${PRIORS_DIR}/prior.cpp
  ${PRIORS_DIR}/prior.h
  ${DISTRIBUTIONS_DIR}/multinomial_dist.h
  ${DISTRIBUTIONS_DIR}/multinomial_dist.cpp
  ${DISTRIBUTIONS_DIR}/mv_gaussian.h
  ${DISTRIBUTIONS_DIR}/mv_gaussian.cpp
  ${DISTRIBUTIONS_UTIL_DIR}/dirichlet.h
  ${DISTRIBUTIONS_UTIL_DIR}/logdet.h
  ${DISTRIBUTIONS_UTIL_DIR}/pdflib.cpp
  ${DISTRIBUTIONS_UTIL_DIR}/pdflib.hpp
  ${DISTRIBUTIONS_UTIL_DIR}/rnglib.cpp
  ${DISTRIBUTIONS_UTIL_DIR}/rnglib.hpp
  ${JSON_DIR}/src/lib_json/json_reader.cpp
  ${JSON_DIR}/src/lib_json/json_value.cpp
  ${JSON_DIR}/src/lib_json/json_writer.cpp
  ${MITOOLBOX_DIR}/src/ArrayOperations.c
  ${MITOOLBOX_DIR}/src/CalculateProbability.c
  ${MITOOLBOX_DIR}/src/Entropy.c
  ${MITOOLBOX_DIR}/src/MutualInformation.c
  ${CNPY_DIR}/cnpy.h
  ${CNPY_DIR}/cnpy.cpp
  )
 
add_subdirectory(jsoncpp)
add_subdirectory(zlib)

#add_library(cnpy SHARED "${CNPY_DIR}/cnpy.cpp")
#target_link_libraries(cnpy ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
#install(TARGETS "cnpy" LIBRARY DESTINATION lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)


if(NOT EXISTS ${ZLIB_INCLUDE_DIR}/zconf.h AND EXISTS ${ZLIB_INCLUDE_DIR}/zconf.h.included)
    message(STATUS "Renaming")
    message(STATUS "    ${ZLIB_INCLUDE_DIR}/zconf.h")
    message(STATUS "to 'zconf.h.included' because this file is included with zlib")
    message(STATUS "but CMake generates it automatically in the build directory.")
    file(RENAME ${ZLIB_INCLUDE_DIR}/zconf.h.included ${ZLIB_INCLUDE_DIR}/zconf.h)
endif()

#add_subdirectory(cnpy)
if (WIN32)
    target_link_libraries(DPMMSubClusters "${CMAKE_SOURCE_DIR}/zlib/out/build/x64-Release/zlib.lib")
endif(WIN32)

if (UNIX)
    target_link_libraries(DPMMSubClusters libz.a)
endif(UNIX)


#add_subdirectory("${CMAKE_SOURCE_DIR}/eigen-3.3.9")

# Request that particles be built with -std=c++11
# As this is a public compile feature anything that links to 
# particles will also build with -std=c++11
#target_compile_features(DPMMSubClusters PUBLIC cxx_std_11)
 
# We need to explicitly state that we need all CUDA files in the 
# particle library to be built with -dc as the member functions 
# could be called by other libraries and executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")


set_target_properties( DPMMSubClusters
                       PROPERTIES 
                       CUDA_SEPARABLE_COMPILATION ON
                   )
#add_executable(particle_test test.cu)
 
#set_property(TARGET particle_test 
#             PROPERTY CUDA_SEPARABLE_COMPILATION ON)
#target_link_libraries(particle_test PRIVATE particles)

if (WIN32)
    target_link_libraries(DPMMSubClusters ${CMAKE_CUBLAS_DIR}/cublas.lib)
	target_link_libraries(DPMMSubClusters ${OpenCV_LIBS})
	target_link_libraries(DPMMSubClusters ${OpenCV_INCLUDE_DIRS}/opencv_world343.lib)
    
    add_custom_command(TARGET DPMMSubClusters POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OpenCV_INCLUDE_DIRS}/opencv_world343.dll"
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
endif(WIN32)

if (UNIX)
    target_link_libraries(DPMMSubClusters ${CMAKE_CUBLAS_DIR}/libcublas.so.11)
    set(DBLAS_LIBDIR "${CMAKE_CUBLAS_DIR}")
endif(UNIX)

target_link_libraries(DPMMSubClusters ${OpenCV_LIBS})
message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS}/opencv2)



set_property(TARGET DPMMSubClusters PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS ON)

SET(GCC_OPEN_MP_FLAGS  "-fopenmp")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_OPEN_MP_FLAGS}")
# SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# install(TARGETS ${PROJECT_NAME}
#         CONFIGURATIONS Release
#         RUNTIME DESTINATION Release/bin)

message(STATUS "CMAKE_BINARY_DIR  = ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY  = ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# add_subdirectory(test)