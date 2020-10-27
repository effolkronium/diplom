add_subdirectory(Render)
add_subdirectory(Scene)
add_subdirectory(Utils)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})

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

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_SOURCE_DIR}/resources/imgui.ini $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/imgui.ini)
