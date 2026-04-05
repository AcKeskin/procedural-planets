# App: camera, UI, orchestration, entry point
add_executable(procedural-planets
    Main.cpp
    Application.cpp
    Input.cpp
)

target_include_directories(procedural-planets PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(procedural-planets PRIVATE
    core
    render
)

# Copy shaders to output directory
add_custom_command(TARGET procedural-planets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:procedural-planets>/shaders
)

# Copy data directory (palettes, body configs)
add_custom_command(TARGET procedural-planets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:procedural-planets>/data
)
