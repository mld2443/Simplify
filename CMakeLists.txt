cmake_minimum_required(VERSION 3.27)
project(simplify)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOURCE_FILES src/main.cpp src/halfedge.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::OpenGL OpenGL::GLU GLUT::GLUT)
