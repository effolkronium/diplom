project(IRender LANGUAGES CXX)

add_subdirectory(RenderOpengl)
add_subdirectory(RenderVulkan)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} 
            INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RenderOpengl PROPERTIES FOLDER ${CMAKE_PROJECT_NAME}/${PROJECT_NAME})
set_target_properties(RenderVulkan PROPERTIES FOLDER ${CMAKE_PROJECT_NAME}/${PROJECT_NAME})