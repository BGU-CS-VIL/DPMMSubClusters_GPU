set(This DPMMSubClustersTest)

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
#add_subdirectory(googletest)

project(${This} LANGUAGES CXX CUDA)

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

set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda-11.2/bin)
#set(PATH ${PATH} ${CUDA_TOOLKIT_ROOT_DIR})
LIST(APPEND CMAKE_PROGRAM_PATH  ${CUDA_TOOLKIT_ROOT_DIR})



#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
#set(OpenCV_DIR ${PROJECT_SOURCE_DIR}/opencv)
#include_directories(${OpenCV_DIR})
#set(${OpenCV_LIBS}  "/home/s/opencv_4_install/lib/")
#find_package(OpenCV)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)

set(OpenCV_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../opencv)
#include_directories(${OpenCV_INCLUDE_DIRS})
#include_directories(googletest/googletest/include)

enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${EIGEN_INCLUDE_DIR})
include_directories(${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

set(CUDA_HELPER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cuda_helper")
include_directories(${CUDA_HELPER_DIR})

set(STATS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../stats/include")
include_directories(${STATS_DIR})

set(GCEM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../gcem/include")
include_directories(${GCEM_DIR})

#set(OpenCV_DIR "${PROJECT_SOURCE_DIR}/opencv")
#include_directories(${OpenCV_DIR})

#set(OPENCV_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/opencv/include")
#include_directories(${OPENCV_INCLUDE_DIR})

#set(OpenCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/opencv/include)
include_directories(${OpenCV_INCLUDE_DIRS})

set(DISTRIBUTIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../distributions")
include_directories(${DISTRIBUTIONS_DIR})

set(DISTRIBUTIONS_UTIL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../distributions_util)
include_directories(${DISTRIBUTIONS_UTIL_DIR})

set(PRIORS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../priors")

add_executable(${This}
  niw_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../clusterInfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../clusterInfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../data_generators.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../data_generators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../distibution_sample.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../distibution_sample.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../dp_parallel_sampling.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../dp_parallel_sampling.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../Draw.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../DPMMSubClusters.cu
  ${CMAKE_CURRENT_SOURCE_DIR}/../DPMMSubClusters.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../local_clusters_actions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../local_clusters_actions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../logger.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../logger.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../module_tests.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../module_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../moduleTypes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../cudaKernel.cu
  ${CMAKE_CURRENT_SOURCE_DIR}/../cudaKernel.cuh
  ${CMAKE_CURRENT_SOURCE_DIR}/../global_params.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../global_params.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared_actions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared_actions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../sufficient_statistics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sufficient_statistics.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../utils.h
  ${PRIORS_DIR}/multinomial_prior.cpp
  ${PRIORS_DIR}/multinomial_prior.h
  ${PRIORS_DIR}/niw.cpp
  ${PRIORS_DIR}/niw.h
  ${PRIORS_DIR}/prior.cpp
  ${PRIORS_DIR}/prior.h
  ${DISTRIBUTIONS_DIR}/multinomial_dist.h
  ${DISTRIBUTIONS_DIR}/multinomial_dist.cpp
  ${DISTRIBUTIONS_DIR}/mv_gaussian.h
  ${DISTRIBUTIONS_DIR}/mv_gaussian.cpp
  ${DISTRIBUTIONS_DIR}/mv_gaussian.h
  ${DISTRIBUTIONS_DIR}/mv_gaussian.cpp
  ${DISTRIBUTIONS_UTIL_DIR}/dirichlet.h
  ${DISTRIBUTIONS_UTIL_DIR}/logdet.h
  ${DISTRIBUTIONS_UTIL_DIR}/pdflib.cpp
  ${DISTRIBUTIONS_UTIL_DIR}/pdflib.hpp
  ${DISTRIBUTIONS_UTIL_DIR}/rnglib.cpp
  ${DISTRIBUTIONS_UTIL_DIR}/rnglib.hpp
  ${GTEST_SRC}

  )


#target_link_libraries( ${This} ${CMAKE_THREAD_LIBS_INIT})

#add_subdirectory(eigen-3.3.9)

# Request that particles be built with -std=c++11
# As this is a public compile feature anything that links to 
# particles will also build with -std=c++11
#target_compile_features(DPMMSubClusters PUBLIC cxx_std_11)
 
# We need to explicitly state that we need all CUDA files in the 
# particle library to be built with -dc as the member functions 
# could be called by other libraries and executables
set_target_properties( ${This}
                       PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

#add_executable(particle_test test.cu)
 
#set_property(TARGET particle_test 
#             PROPERTY CUDA_SEPARABLE_COMPILATION ON)
#target_link_libraries(particle_test PRIVATE particles)

#target_link_libraries(${This} gtest_main)
target_link_libraries(${This} Eigen3::Eigen)

#target_link_libraries(${This} opencv_world343)

target_link_libraries(${This} ${OpenCV_LIBS})

target_link_libraries(${This} ${OpenCV_INCLUDE_DIRS}/opencv_world343.lib)

target_link_libraries(${This} gtest gtest_main)


add_custom_command(TARGET ${This} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OpenCV_INCLUDE_DIRS}/opencv_world343.dll"
        $<TARGET_FILE_DIR:${This}>)


set_property(TARGET ${This} PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS ON)


add_test(
	NAME ${This}
	COMMAND ${This}
)

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
