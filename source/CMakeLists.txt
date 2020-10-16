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

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/resources.zip")
	file(DOWNLOAD ${DIPLOM_RESOURCES_ZIP_URL} "${CMAKE_SOURCE_DIR}/resources.zip" SHOW_PROGRESS)
	file(REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/resources")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/resources)
	execute_process(COMMAND "${DIPLOM_TOOLS_DIR}/7z.exe" x "${CMAKE_SOURCE_DIR}/resources.zip" -o${CMAKE_SOURCE_DIR}/)
endif()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/resources)