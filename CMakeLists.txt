cmake_minimum_required(VERSION 3.16)

project(yaglt
    VERSION 0.1.0
    DESCRIPTION "Yet Another GL Translator - backend-agnostic OpenGL 4.6 compatibility layer"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(YAGLT_BUILD_TESTS "Build the unit/integration test suite" ON)
option(YAGLT_ENABLE_SANITIZERS "Enable ASan/UBSan in Debug builds" OFF)

if(YAGLT_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(src)
if(YAGLT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
