project(RenderOpengl LANGUAGES CXX)

file(GLOB SHADERS Shaders/*)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADERS})

add_library(${PROJECT_NAME} STATIC 
            Window.h
            Window.cpp
   
            RenderOpengl.cpp
            RenderOpengl.h

            Shader.cpp
            Shader.h

            Camera.cpp
            Camera.h

            Shaders.h

            Mesh.h
            Mesh.cpp

            Model.h
            Model.cpp

            ../IRender.h

            ${SHADERS}

            ${CMAKE_SOURCE_DIR}/external/glad_opengl4.6_core/src/glad.c
)

target_link_libraries(${PROJECT_NAME}
            PUBLIC
                IRender
            PRIVATE
                OpenGL::GL
                glfw
                glm
                assimp::assimp
                Utils
)

target_include_directories(${PROJECT_NAME} 
            INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}
            PRIVATE
                ${CMAKE_SOURCE_DIR}/external/glad_opengl4.6_core/include
                ${CMAKE_SOURCE_DIR}/external/
                ${CMAKE_SOURCE_DIR}/external/assimp-5.0.1/include
)


# Shaders processing for c++ include

function(make_includable input_file output_path include_file)
    get_filename_component(input_file_name ${input_file} NAME)
    get_filename_component(input_file_name_WLE ${input_file} NAME_WE)

    file(READ ${input_file} content)
    set(delim "for_c++_include")
    set(content "R\"${delim}(\n${content})${delim}\"")
    file(WRITE ${output_path}/${input_file_name} "${content}")

    file(APPEND "Shaders.h" "inline constexpr char* const s_${input_file_name_WLE} = \n"
                             "#include \"${output_path}${input_file_name}\"\n;\n")    
endfunction(make_includable)

file(REMOVE Shaders.h)
file(REMOVE_RECURSE ShadersGen)

foreach(shader ${SHADERS})
    make_includable(${shader} ShadersGen/ Shaders.h)
endforeach()

