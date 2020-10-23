project(RenderVulkan LANGUAGES CXX)

file(GLOB SHADERS Shaders/*)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADERS})

file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/ShadersGen/Shaders.h")
file(REMOVE_RECURSE ShadersGen)

file(MAKE_DIRECTORY ShadersGen)

file(APPEND "${CMAKE_CURRENT_SOURCE_DIR}/ShadersGen/Shaders.h" "#include <array>\n\n")

foreach(shader ${SHADERS})
    get_filename_component(shaderName ${shader} NAME)
    get_filename_component(shaderNameWLE ${shader} NAME_WE)
    get_filename_component(shaderNameEXT ${shader} EXT)
    string(REGEX REPLACE "\\." "_" shaderNameCppName "${shaderName}")

    set(shader_bin "${CMAKE_CURRENT_SOURCE_DIR}/ShadersGen/${shaderName}.bin")
    execute_process(COMMAND "${DIPLOM_SHADER_COMPILER}" "${shader}" "-o${shader_bin}" COMMAND_ECHO STDOUT)
    file(READ ${shader_bin} SHADER_BIN_CONTENT HEX)

    string(LENGTH ${SHADER_BIN_CONTENT} SHADER_BIN_CONTENT_CPP_ARRAY_LENGTH)
    math(EXPR SHADER_BIN_CONTENT_CPP_ARRAY_LENGTH "${SHADER_BIN_CONTENT_CPP_ARRAY_LENGTH} / 2" OUTPUT_FORMAT DECIMAL)

    string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1," SHADER_BIN_CONTENT_CPP_ARRAY "${SHADER_BIN_CONTENT}")
    
    
    set(SHADER_BIN_CONTENT_CPP_ARRAY "constexpr static inline std::array<unsigned char, ${SHADER_BIN_CONTENT_CPP_ARRAY_LENGTH}> s_${shaderNameCppName} {${SHADER_BIN_CONTENT_CPP_ARRAY}};")
    
    file(APPEND "${CMAKE_CURRENT_SOURCE_DIR}/ShadersGen/Shaders.h" "\n${SHADER_BIN_CONTENT_CPP_ARRAY}\n")
endforeach()

add_library(${PROJECT_NAME} STATIC 
            ${SHADERS}

            RenderVulkan.cpp
            RenderVulkan.h
            ../IRender.h
			ShadersGen/Shaders.h
			${CMAKE_SOURCE_DIR}/external/glad_vulkan1.2_core/src/vulkan.c
)

source_group("Shaders" FILES ${SHADERS})

target_link_libraries(${PROJECT_NAME}
            PUBLIC
                IRender
            PRIVATE
                glfw
                glm
				Utils
)

target_include_directories(${PROJECT_NAME} 
            INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(${PROJECT_NAME} SYSTEM
			PRIVATE
				${CMAKE_SOURCE_DIR}/external/glad_vulkan1.2_core/include
                ${CMAKE_SOURCE_DIR}/external/
                ${CMAKE_SOURCE_DIR}/external/assimp-5.0.1/include
)