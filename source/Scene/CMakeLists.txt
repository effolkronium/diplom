project(Scene LANGUAGES CXX)

add_subdirectory(SceneSimple)

add_library(${PROJECT_NAME} STATIC
            Scene.h
            Scene.cpp
)

target_link_libraries(${PROJECT_NAME}
            PRIVATE
                SceneSimple
                IRender
)

target_include_directories(${PROJECT_NAME}
            INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${CMAKE_PROJECT_NAME}/${PROJECT_NAME})
set_target_properties(SceneSimple PROPERTIES FOLDER ${CMAKE_PROJECT_NAME}/${PROJECT_NAME})