cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(MIPS-Simulator-Dev CXX)

add_executable(${PROJECT_NAME}
    emulator.cpp)

target_link_libraries(${PROJECT_NAME}
    binaryninjaapi)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME}
    dl)
endif()

set(CMAKE_CXX_FLAGS "-fpermissive -g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS "-g -fsanitize=address -fno-omit-frame-pointer")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_VISIBILITY_PRESET hidden
    CXX_STANDARD_REQUIRED ON
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)
