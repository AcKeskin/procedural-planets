# EmbedShaders.cmake — Converts GLSL source files into C++ string constants
# Usage: cmake -DSHADER_DIR="path/to/shaders" -DOUTPUT_FILE="shaders.h" -P EmbedShaders.cmake

if(NOT DEFINED SHADER_DIR OR NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "SHADER_DIR and OUTPUT_FILE must be defined")
endif()

# Find all shader files
file(GLOB COMP_FILES "${SHADER_DIR}/compute/*.comp")
file(GLOB GLSL_FILES "${SHADER_DIR}/includes/*.glsl")
set(ALL_SHADERS ${COMP_FILES} ${GLSL_FILES})

# Header
set(CONTENT "#pragma once\n\n// Auto-generated embedded shader strings — do not edit\n\n#include <string>\n\nnamespace planetgen {\nnamespace shaders {\n\n")

foreach(SHADER_FILE IN LISTS ALL_SHADERS)
    file(READ "${SHADER_FILE}" FILE_CONTENT)

    get_filename_component(FILENAME "${SHADER_FILE}" NAME)
    string(REPLACE "." "_" VAR_NAME "${FILENAME}")

    # Escape backslashes, then quotes, then newlines
    string(REPLACE "\\" "\\\\" FILE_CONTENT "${FILE_CONTENT}")
    string(REPLACE "\"" "\\\"" FILE_CONTENT "${FILE_CONTENT}")
    # Replace newlines with escaped newline + string continuation
    string(REGEX REPLACE "\n" "\\\\n\"\n    \"" FILE_CONTENT "${FILE_CONTENT}")

    string(APPEND CONTENT "inline const std::string ${VAR_NAME} =\n    \"${FILE_CONTENT}\";\n\n")
endforeach()

string(APPEND CONTENT "} // namespace shaders\n} // namespace planetgen\n")

file(WRITE "${OUTPUT_FILE}" "${CONTENT}")
