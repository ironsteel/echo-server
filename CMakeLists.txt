project(echo-server)
cmake_minimum_required(VERSION 3.10)

aux_source_directory("${PROJECT_SOURCE_DIR}/src/." SOURCES_LIST)

add_executable(${PROJECT_NAME} ${SOURCES_LIST})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
