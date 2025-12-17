cmake_minimum_required(VERSION 3.16)
project(ObjViewer C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(external/glfw)

add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(ObjViewer
    src/main.c
)

target_include_directories(ObjViewer PUBLIC
    external/glad/include
    external/glfw/include
)

target_link_libraries(ObjViewer PRIVATE glfw glad)

if (WIN32)
    target_link_libraries(ObjViewer PRIVATE opengl32)
elseif(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    target_link_libraries(ObjViewer PRIVATE ${OpenGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(ObjViewer PRIVATE OpenGL::GL)
endif()
