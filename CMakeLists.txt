cmake_minimum_required(VERSION 3.27)
project(simplify)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOURCE_FILES src/main.cpp src/collapsible.cpp src/halfedge.cpp src/manifold.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::OpenGL GLUT::GLUT)
