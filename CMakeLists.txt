cmake_minimum_required(VERSION 3.15...4.1)

project(
  ProjectFramework
  VERSION 0.0.1
  LANGUAGES C CXX) # C necessary for GLAD

set(CMAKE_CXX_STANDARD 17)

add_executable(ProjectFramework 
  src/main.cpp
  src/ext/glad.c
)

target_include_directories(ProjectFramework PRIVATE
  include
)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(ProjectFramework PRIVATE
  OpenGL::GL
  glfw3
)