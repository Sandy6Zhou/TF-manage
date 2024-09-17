cmake_minimum_required(VERSION 3.8)
project(TF-manage_test LANGUAGES C CXX)

add_compile_options(-O2 -Wall -std=c99 -D _GNU_SOURCE)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -s")

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCS_1)
aux_source_directory(${PROJECT_SOURCE_DIR}/test SRCS_2)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} ${SRCS_1} ${SRCS_2})

install(TARGETS ${PROJECT_NAME} DESTINATION ${PROJECT_SOURCE_DIR}/bin)
