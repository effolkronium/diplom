project(imgui LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC 
            imgui.cpp
            imgui_draw.cpp
            imgui_widgets.cpp
            backends/imgui_impl_glfw.cpp
            backends/imgui_impl_opengl3.cpp
)

target_include_directories(${PROJECT_NAME} 
            PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/backends
                ${CMAKE_SOURCE_DIR}/external/glad_opengl4.6_core/include
)

target_link_libraries(${PROJECT_NAME}
            PRIVATE
                OpenGL::GL
                glfw
)
