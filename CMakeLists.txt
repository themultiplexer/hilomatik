cmake_minimum_required(VERSION 3.5)
project(hilomatik)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF (!WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX -Wall -Wextra -Werror -g -Wno-unused-parameter")
ENDIF()

find_package(X11 REQUIRED)
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
IF (WIN32)
find_package(unofficial-kissfft REQUIRED)
ELSE()
find_package(kissfft REQUIRED)
ENDIF()

find_package(RtAudio REQUIRED)

add_compile_definitions(_USE_MATH_DEFINES)
add_executable(hilomatik main.cpp)

#include_directories(${X11_INCLUDE_DIR})
#include_directories(${OPENGL_INCLUDE_DIRS})
#include_directories(${GLEW_INCLUDE_DIRS})
IF (WIN32)
target_link_libraries(hilomatik PRIVATE ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}  glfw RtAudio::rtaudio unofficial::kissfft::kissfft)
ELSE()
target_link_libraries(hilomatik PRIVATE ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}  glfw RtAudio::rtaudio "/usr/lib/x86_64-linux-gnu/libkissfft-float.so.131.1.0")
ENDIF()