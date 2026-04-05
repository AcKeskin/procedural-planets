# EmbedShaders.cmake — Converts GLSL source files into C++ string constants
# Resolves #include "filename" directives by inlining from the includes/ directory.
# Usage: cmake -DSHADER_DIR="path/to/shaders" -DOUTPUT_FILE="shaders.h" -P EmbedShaders.cmake

if(NOT DEFINED SHADER_DIR OR NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "SHADER_DIR and OUTPUT_FILE must be defined")
endif()

# Resolve #include "filename" by replacing with contents from includes/
function(resolve_includes INPUT_STRING INCLUDE_DIR OUT_VAR)
    set(RESULT "${INPUT_STRING}")

    # Keep resolving until no more includes (handles nested includes)
    set(MAX_DEPTH 10)
    foreach(DEPTH RANGE 1 ${MAX_DEPTH})
        string(FIND "${RESULT}" "#include" HAS_INCLUDE)
        if(HAS_INCLUDE EQUAL -1)
            break()
        endif()

        # Match #include "filename"
        string(REGEX MATCH "#include[ \t]+\"([^\"]+)\"" MATCH_FULL "${RESULT}")
        if(NOT MATCH_FULL)
            break()
        endif()
        string(REGEX REPLACE "#include[ \t]+\"([^\"]+)\"" "\\1" INCLUDE_FILE "${MATCH_FULL}")

        # Read the included file
        set(INCLUDE_PATH "${INCLUDE_DIR}/${INCLUDE_FILE}")
        if(NOT EXISTS "${INCLUDE_PATH}")
            message(WARNING "Include file not found: ${INCLUDE_PATH}")
            break()
        endif()

        file(READ "${INCLUDE_PATH}" INCLUDE_CONTENT)

        # Replace the #include directive with the file contents
        string(REPLACE "${MATCH_FULL}" "${INCLUDE_CONTENT}" RESULT "${RESULT}")
    endforeach()

    set(${OUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()

# Only embed compute shaders (not the include files themselves)
file(GLOB COMP_FILES "${SHADER_DIR}/compute/*.comp")

# Header
set(CONTENT "#pragma once\n\n// Auto-generated embedded shader strings — do not edit\n// Includes have been resolved inline.\n\n#include <string>\n\nnamespace planetgen {\nnamespace shaders {\n\n")

foreach(SHADER_FILE IN LISTS COMP_FILES)
    file(READ "${SHADER_FILE}" FILE_CONTENT)

    # Resolve #include directives
    resolve_includes("${FILE_CONTENT}" "${SHADER_DIR}/includes" FILE_CONTENT)

    get_filename_component(FILENAME "${SHADER_FILE}" NAME)
    string(REPLACE "." "_" VAR_NAME "${FILENAME}")

    # Escape backslashes, then quotes, then newlines
    string(REPLACE "\\" "\\\\" FILE_CONTENT "${FILE_CONTENT}")
    string(REPLACE "\"" "\\\"" FILE_CONTENT "${FILE_CONTENT}")
    string(REGEX REPLACE "\n" "\\\\n\"\n    \"" FILE_CONTENT "${FILE_CONTENT}")

    string(APPEND CONTENT "inline const std::string ${VAR_NAME} =\n    \"${FILE_CONTENT}\";\n\n")
endforeach()

string(APPEND CONTENT "} // namespace shaders\n} // namespace planetgen\n")

file(WRITE "${OUTPUT_FILE}" "${CONTENT}")
