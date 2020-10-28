cmake_minimum_required(VERSION 3.12)

project(diplom LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (WIN32)
	add_definitions(-DUNICODE -D_UNICODE)
endif()

set(DIPLOM_TOOLS_DIR "${CMAKE_SOURCE_DIR}/tools")

if (WIN32)
    set(DIPLOM_SHADER_COMPILER "${CMAKE_SOURCE_DIR}/tools/glslc.exe")
else()
    set(DIPLOM_SHADER_COMPILER "${CMAKE_SOURCE_DIR}/tools/glslc")
endif()

set(DIPLOM_RESOURCES_ZIP_URL https://api.onedrive.com/v1.0/shares/u!aHR0cHM6Ly8xZHJ2Lm1zL3UvcyFBazltZDlNYkhlUlRocmhzMFZGa2k2U2VIYjZ4bXc_ZT1peFFH/root/content)


set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw-3.3.2)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glm)

SET(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
SET(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/assimp-5.0.1)

set_target_properties(glfw PROPERTIES FOLDER external/glfw)
set_target_properties(uninstall PROPERTIES FOLDER external/glfw)

get_target_property(ASSIMP_DEPS assimp::assimp LINK_LIBRARIES)

if(WIN32)
    set_target_properties(${ASSIMP_DEPS} PROPERTIES FOLDER external/assimp)
    set_target_properties(zlib PROPERTIES FOLDER external/assimp)
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER external/assimp)
endif()
set_target_properties(assimp PROPERTIES FOLDER external/assimp)

find_package(OpenGL REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui-1.79)
set_target_properties(imgui PROPERTIES FOLDER external/imgui)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}) 

add_subdirectory(source)
