# Test suite. All tests run headlessly with the mock backend so the project
# is testable on standard Linux CI without an Android device or GPU.

add_executable(yaglt_tests
    unit/main.cpp
    unit/capabilities_test.cpp
    unit/backend_test.cpp
)

target_include_directories(yaglt_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)

target_link_libraries(yaglt_tests PRIVATE yaglt_core)

add_test(NAME yaglt_tests COMMAND yaglt_tests)
