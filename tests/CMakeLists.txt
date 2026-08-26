# Test suite. All tests run headlessly with the mock backend so the project
# is testable on standard Linux CI without an Android device or GPU.

add_executable(yaglt_tests
    unit/main.cpp
    unit/capabilities_test.cpp
    unit/backend_test.cpp
    unit/object_test.cpp
    unit/gl_api_test.cpp
    unit/state_test.cpp
    unit/state_flush_test.cpp
    unit/ubo_ssbo_test.cpp
    unit/draw_test.cpp
    unit/shader_program_test.cpp
    unit/texture_fbo_test.cpp
    unit/texture_unit_test.cpp
    unit/dsa_texture_test.cpp
    unit/texparam_query_test.cpp
    unit/texsubimage_test.cpp
    unit/framebuffer_buf_test.cpp
    unit/uniform_test.cpp
    unit/android_capabilities_test.cpp
    unit/getstring_test.cpp
    unit/shader_stage_test.cpp
    unit/shader_program_query_test.cpp
    unit/infolog_test.cpp
    unit/glsl_version_check_test.cpp
    unit/transform_feedback_test.cpp
    unit/sampler_test.cpp
    unit/buffer_completeness_test.cpp
    unit/viewport_scissor_test.cpp
    unit/clear_test.cpp
    unit/blend_test.cpp
    unit/stencil_test.cpp
    unit/raster_test.cpp
    unit/readpixels_test.cpp
    unit/log_test.cpp
    unit/getstate_test.cpp
    backend/gles_backend_test.cpp
)

if(YAGLT_SHADER_TRANSLATE)
    target_sources(yaglt_tests PRIVATE
        backend/shader_translate_test.cpp
        backend/gles_e2e_shader_test.cpp
        backend/gles_e2e_program_test.cpp
    )
endif()

target_include_directories(yaglt_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)

target_link_libraries(yaglt_tests PRIVATE yaglt_core)

add_test(NAME yaglt_tests COMMAND yaglt_tests)
