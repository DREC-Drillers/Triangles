execute_process(
    COMMAND "${TRIANGLES_EXECUTABLE}" "${TRIANGLES_INPUT}" --no-visualization
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
)

if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Application failed (${result}):\n${error_output}")
endif()

if(NOT output STREQUAL "0\n1\n2\n3\n")
    message(FATAL_ERROR "Unexpected intersecting polygon indices: '${output}'")
endif()
