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
    unit/draw_expansion_test.cpp
    unit/shader_program_test.cpp
    unit/bind_attrib_location_test.cpp
    unit/transform_feedback_varyings_test.cpp
    unit/provoking_vertex_test.cpp
    unit/clamp_color_test.cpp
    unit/indirect_draw_test.cpp
    unit/vertex_attrib_generic_test.cpp
    unit/hint_test.cpp
    unit/dither_test.cpp
    unit/srgb_alpha_coverage_test.cpp
    unit/indexed_caps_test.cpp
    unit/texture_fbo_test.cpp
    unit/teximage_cube_test.cpp
    unit/texture_unit_test.cpp
    unit/dsa_texture_test.cpp
    unit/dsa_named_texture_test.cpp
    unit/dsa_named_framebuffer_test.cpp
    unit/dsa_vertex_array_test.cpp
    unit/vertex_attrib_binding_test.cpp
    unit/vertex_array_test.cpp
    unit/texparam_query_test.cpp
    unit/texparam_int_invalidate_test.cpp
    unit/texsubimage_test.cpp
    unit/texture_storage_test.cpp
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
    unit/bind_samplers_test.cpp
    unit/bind_buffers_multi_test.cpp
    unit/query_sync_test.cpp
    unit/buffer_completeness_test.cpp
    unit/buffer_parameter_i64_test.cpp
    unit/internalformat_query_test.cpp
    unit/viewport_scissor_test.cpp
    unit/clear_test.cpp
    unit/clear_stencil_test.cpp
    unit/blend_test.cpp
    unit/indexed_blend_test.cpp
    unit/viewport_scissor_depth_array_test.cpp
    unit/colormaski_test.cpp
    unit/stencil_test.cpp
    unit/compute_dispatch_test.cpp
    unit/compute_shader_object_test.cpp
    unit/texture_view_test.cpp
    unit/raster_test.cpp
    unit/readpixels_test.cpp
    unit/logicop_blit_invalidate_test.cpp
    unit/colormask_test.cpp
    unit/samplecoverage_test.cpp
    unit/dsa_program_pipeline_test.cpp
    unit/program_parameter_test.cpp
    unit/shader_binary_test.cpp
    unit/rasterization_control_test.cpp
    unit/polygon_offset_clamp_test.cpp
    unit/patch_parameter_test.cpp
    unit/program_resource_test.cpp
    unit/subroutine_test.cpp
    unit/active_uniform_attrib_test.cpp
    unit/program_stage_test.cpp
    unit/primitive_restart_test.cpp
    unit/log_test.cpp
    unit/getstate_test.cpp
    unit/generic_query_test.cpp
    unit/reflection_query_test.cpp
    unit/object_label_test.cpp
    unit/frag_data_location_test.cpp
    unit/detach_shader_test.cpp
    unit/transform_feedback_varying_test.cpp
    unit/memory_barrier_test.cpp
    unit/program_interface_test.cpp
    unit/compressed_tex_image_test.cpp
    unit/tex_image_robustness_test.cpp
    unit/query_buffer_object_test.cpp
    unit/indexed_query_test.cpp
    unit/texture_sub_image_test.cpp
    unit/conditional_render_test.cpp
    unit/uniform_block_binding_test.cpp
    unit/transform_feedback_buffer_test.cpp
    unit/create_object_test.cpp
    unit/object_is_test.cpp
    unit/point_parameter_test.cpp
    unit/clear_buffer_test.cpp
    unit/clip_control_test.cpp
    unit/viewport_indexed_test.cpp
    backend/gles_backend_test.cpp
)

if(YAGLT_SHADER_TRANSLATE)
    target_sources(yaglt_tests PRIVATE
        backend/shader_translate_test.cpp
        backend/gles_e2e_shader_test.cpp
        backend/gles_e2e_program_test.cpp
        backend/gles_e2e_1d_texture_test.cpp
        backend/gles_e2e_internalformat_test.cpp
    )
endif()

target_include_directories(yaglt_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)

target_link_libraries(yaglt_tests PRIVATE yaglt_core)

add_test(NAME yaglt_tests COMMAND yaglt_tests)

# libEGL drop-in shim smoke test (see src/egl_shim). Links the shim directly so
# its egl*/gl* symbols are resolved, then drives a real (Mesa) EGL stack.
if(YAGLT_EGL_SHIM)
    add_executable(egl_shim_smoke
        integration/egl_shim_smoke.cpp
    )
    target_include_directories(egl_shim_smoke PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(egl_shim_smoke PRIVATE yaglt_egl_shim)
endif()
