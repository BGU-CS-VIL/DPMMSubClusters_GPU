if (WIN32)
    set(CMAKE_CUDA_COMPILER "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/bin/nvcc.exe")
    set(CMAKE_CUBLAS_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/lib/x64")
endif (WIN32)

if (UNIX)
    set(CMAKE_CUDA_COMPILER "/usr/local/cuda-11.3/bin/nvcc")
    set(CMAKE_CUBLAS_DIR "/usr/local/cuda-11.3/lib64")
endif (UNIX)

set(PROJECT_NAME "DPMMSubClusters")
set(CMAKE_PROJECT_NAME ${PROJECT_NAME})
cmake_policy(SET CMP0048 NEW)
project(${CMAKE_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX CUDA C)

cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckCXXCompilerFlag)
include(GNUInstallDirs)

LIST(APPEND CMAKE_PROGRAM_PATH  ${CUDA_TOOLKIT_ROOT_DIR})

set(EIGEN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/eigen-3.3.9)

message(STATUS "Eigen:")
add_subdirectory("${CMAKE_SOURCE_DIR}/eigen-3.3.9")

message(STATUS "OpenCV:")

if (WIN32)
    file (REMOVE "${CMAKE_SOURCE_DIR}/install/opencv//OpenCVConfig.cmake"})
    LIST(APPEND CMAKE_PREFIX_PATH  "${CMAKE_SOURCE_DIR}/install/opencv/x64/vc16/lib")
    find_package(OpenCV QUIET COMPONENTS core highgui imgproc imgcodecs)
    if(NOT OpenCV_FOUND)
        message(FATAL_ERROR "run \"./installOCV.sh\" using Git Bash from ${CMAKE_SOURCE_DIR}")
    endif()    
    
    include_directories(${OpenCV_INCLUDE_DIRS})
    
    message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")

endif (WIN32)


include_directories(${EIGEN_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/eigen-3.3.9)
include_directories(${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

set(STATS_DIR "${CMAKE_SOURCE_DIR}/stats/include")
include_directories(${STATS_DIR})

set(GCEM_DIR "${CMAKE_SOURCE_DIR}/gcem/include")
include_directories(${GCEM_DIR})

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
  ${CMAKE_SOURCE_DIR}/IJsonSerializable.cpp
  ${CMAKE_SOURCE_DIR}/IJsonSerializable.h
  ${CMAKE_SOURCE_DIR}/jsonSerializer.cpp
  ${CMAKE_SOURCE_DIR}/jsonSerializer.h
  ${CMAKE_SOURCE_DIR}/modelInfo.cpp
  ${CMAKE_SOURCE_DIR}/modelInfo.h
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

if (WIN32)
	target_link_libraries(DPMMSubClusters ${OpenCV_LIBS})
endif(WIN32)

if(NOT EXISTS ${ZLIB_INCLUDE_DIR}/zconf.h AND EXISTS ${ZLIB_INCLUDE_DIR}/zconf.h.included)
    message(STATUS "Renaming")
    message(STATUS "    ${ZLIB_INCLUDE_DIR}/zconf.h")
    message(STATUS "to 'zconf.h.included' because this file is included with zlib")
    message(STATUS "but CMake generates it automatically in the build directory.")
    file(RENAME ${ZLIB_INCLUDE_DIR}/zconf.h.included ${ZLIB_INCLUDE_DIR}/zconf.h)
endif()

if (WIN32)
    target_link_libraries(DPMMSubClusters "${CMAKE_SOURCE_DIR}/zlib/out/build/x64-Release/zlib.lib")
endif(WIN32)

if (UNIX)
    target_link_libraries(DPMMSubClusters libz.a)
endif(UNIX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")


set_target_properties( DPMMSubClusters
                       PROPERTIES 
                       CUDA_SEPARABLE_COMPILATION ON
                   )

if (WIN32)
    target_link_libraries(DPMMSubClusters ${CMAKE_CUBLAS_DIR}/cublas.lib)
    
    add_custom_command(TARGET DPMMSubClusters POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/install/opencv/x64/vc16/bin/opencv_world452.dll
        ${CMAKE_BINARY_DIR}/Release)

    add_custom_command(TARGET DPMMSubClusters POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/install/opencv/x64/vc16/bin/opencv_world452d.dll
        ${CMAKE_BINARY_DIR}/Debug)
endif(WIN32)

if (UNIX)
    target_link_libraries(DPMMSubClusters ${CMAKE_CUBLAS_DIR}/libcublas.so.11)
    set(DBLAS_LIBDIR "${CMAKE_CUBLAS_DIR}")
endif(UNIX)

set_property(TARGET DPMMSubClusters PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS ON)

if (UNIX)
    SET(GCC_OPEN_MP_FLAGS  "-fopenmp")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_OPEN_MP_FLAGS}")
endif(UNIX)

message(STATUS "CMAKE_BINARY_DIR  = ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY  = ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")