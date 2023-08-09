cmake_minimum_required(VERSION 3.0)
project(triangle)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX -Wall -Wextra -Werror -g")

find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(triangle main.cpp)

target_link_libraries(triangle ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})