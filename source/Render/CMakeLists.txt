project(Render LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC 
    Camera.h

    Mesh.h
    Mesh.cpp

    Model.h
    Model.cpp

    Render.h
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glm
		Utils
)

target_include_directories(${PROJECT_NAME} 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/external/
        ${CMAKE_SOURCE_DIR}/external/assimp-5.0.1/include
        ${CMAKE_BINARY_DIR}/external/assimp-5.0.1/include
)



add_subdirectory(RenderOpengl)
add_subdirectory(RenderVulkan)

set_target_properties(Render PROPERTIES FOLDER ${CMAKE_PROJECT_NAME}/${PROJECT_NAME})
set_target_properties(RenderOpengl PROPERTIES FOLDER ${CMAKE_PROJECT_NAME}/${PROJECT_NAME})
set_target_properties(RenderVulkan PROPERTIES FOLDER ${CMAKE_PROJECT_NAME}/${PROJECT_NAME})