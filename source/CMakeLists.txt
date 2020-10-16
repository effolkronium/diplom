add_subdirectory(Render)
add_subdirectory(Scene)
add_subdirectory(Utils)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} 
            PRIVATE  
                RenderVulkan
                RenderOpengl
                Scene
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/resources)