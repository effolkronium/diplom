project(RenderVulkan LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC 
            RenderVulkan.cpp
            RenderVulkan.h
            ../IRender.h

			${CMAKE_SOURCE_DIR}/external/glad_vulkan1.2_core/src/vulkan.c
)

target_link_libraries(${PROJECT_NAME}
            PUBLIC
                IRender
            PRIVATE
                glfw
                glm
)

target_include_directories(${PROJECT_NAME} 
            INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}
			PRIVATE
				${CMAKE_SOURCE_DIR}/external/glad_vulkan1.2_core/include
                ${CMAKE_SOURCE_DIR}/external/
                ${CMAKE_SOURCE_DIR}/external/assimp-5.0.1/include
)