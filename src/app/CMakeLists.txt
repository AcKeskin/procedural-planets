# App: camera, UI, orchestration, entry point
add_executable(procedural-planets
    Main.cpp
)

target_link_libraries(procedural-planets PRIVATE
    core
    render
)
