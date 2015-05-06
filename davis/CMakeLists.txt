cmake_minimum_required(VERSION 2.8)

project(Davis)

add_definitions(-std=c++)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(${PROJECT_NAME} SHARED
	davis.c
)

set_source_files_properties(davis.c PROPERTIES COMPILE_FLAGS "-std=c99")

target_link_libraries(${PROJECT_NAME}
	pthread
)
