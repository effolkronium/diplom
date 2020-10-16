project(Utils LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
            Utils.h
            Utils.cpp
)

target_include_directories(${PROJECT_NAME}
            INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${CMAKE_PROJECT_NAME})