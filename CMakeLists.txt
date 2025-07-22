cmake_minimum_required(VERSION 3.25)
project(gpu~)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PDCMAKE_FILE ${CMAKE_BINARY_DIR}/pd.cmake)
set(PDCMAKE_VERSION "v0.2.1")
if(NOT EXISTS "${PDCMAKE_FILE}")
  file(
    DOWNLOAD
    https://raw.githubusercontent.com/pure-data/pd.cmake/refs/tags/${PDCMAKE_VERSION}/pd.cmake
    ${PDCMAKE_FILE})
endif()
include(${PDCMAKE_FILE})

# add_executable(compute_math main.cpp) target_link_libraries(compute_math
# PRIVATE GL GLEW glfw)
pd_add_external(gpu~ gpu~.cpp LINK_LIBRARIES glfw GL GLEW)
pd_add_datafile(gpu~ gpu~-help.pd)
pd_add_datafile(gpu~ compute.glsl)
pd_add_datafile(gpu~ osc~.glsl)
