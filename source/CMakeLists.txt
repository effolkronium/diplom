add_subdirectory(Render)
add_subdirectory(Scene)
add_subdirectory(Utils)

if (NOT WIN32)
    add_executable(${CMAKE_PROJECT_NAME} main.cpp)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -mwindows)
else()
    add_executable(${CMAKE_PROJECT_NAME} WIN32 main.cpp)
    set(ICON_LINK_RES "${CMAKE_SOURCE_DIR}/resources/icon.res")
endif()

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})

target_link_libraries(${CMAKE_PROJECT_NAME}
       	PRIVATE
                RenderVulkan
                RenderOpengl
                Scene
                ${ICON_LINK_RES}
)

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/resources.zip")
	file(DOWNLOAD ${DIPLOM_RESOURCES_ZIP_URL} "${CMAKE_SOURCE_DIR}/resources.zip" SHOW_PROGRESS)
	file(REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/resources")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/resources)
	if(WIN32)
	    execute_process(COMMAND "${DIPLOM_TOOLS_DIR}/7z.exe" x "${CMAKE_SOURCE_DIR}/resources.zip" -o${CMAKE_SOURCE_DIR}/)
        else()
      	    execute_process(COMMAND "unzip" -o "${CMAKE_SOURCE_DIR}/resources.zip" -d "${CMAKE_SOURCE_DIR}/") 
        endif()
endif()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/resources)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_SOURCE_DIR}/resources/imgui.ini $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/imgui.ini)
