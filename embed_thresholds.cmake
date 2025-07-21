file(READ "${INPUT_FILE}" content)
set(delim "for_cpp_include")
set(output "const char* default_thresholds = R\"${delim}(\n${content})${delim}\";")
file(WRITE "${OUTPUT_FILE}" "${output}")
