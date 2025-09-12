cmake_minimum_required(VERSION 3.20)

project(luao VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(SOURCES
    src/api.cpp
    src/bytecode.cpp
    src/debug.cpp
    src/lexer.cpp
    src/luao.cpp
    src/object.cpp
    src/parser.cpp
    src/table.cpp
    src/vm.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(luao PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
