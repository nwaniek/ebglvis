cmake_minimum_required(VERSION 2.8)

project(ebglvis)

add_definitions(-std=c++11)
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set(EDVSTOOLS_PATH  "contrib/edvstools")

set(ebglvis_SOURCES
	main.cpp
	shader.cpp
	glutil.cpp
)
set(ebglvis_HEADERS
	shader.hpp
	glutil.hpp
)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)
find_package(
	Boost 1.55.0
	COMPONENTS
		program_options
	REQUIRED)

find_library(EDVS_LIBRARY Edvs ${EDVSTOOLS_PATH}/build/bin)

include_directories(
	${GLFW_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
	${EDVSTOOLS_PATH}
)

add_executable(${PROJECT_NAME}
	${ebglvis_SOURCES}
	${ebglvis_HEADERS}
)

target_link_libraries(${PROJECT_NAME}
	m
	pthread
	${OPENGL_LIBRARIES}
	${GLFW_LIBRARIES}
	${GLEW_LIBRARIES}
	${EDVS_LIBRARY}
	${Boost_LIBRARIES}
)

add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ln -sf ${CMAKE_SOURCE_DIR}/shaders
)
