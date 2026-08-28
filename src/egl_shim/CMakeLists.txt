# libEGL drop-in shim (see docs/architecture.md "Planned: libEGL.so drop-in wrapper").
#
# Builds a shared library named libEGL.so that forwards EGL to the real system
# driver and routes gl* into YAGLT's desktop-GL frontend. Requires the GLES
# backend (it is what the shim drives).

if(NOT YAGLT_BACKEND_GLES)
    message(WARNING "yaglt_egl_shim requires YAGLT_BACKEND_GLES=ON; skipping.")
    return()
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Generate C gl* wrappers from the canonical frontend API header so the GL
# surface is never hand-maintained.
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gl_exports.cpp
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tools/gen_gl_exports.py
            ${CMAKE_SOURCE_DIR}/include/glcompat/frontend/gl_api.hpp
            ${CMAKE_CURRENT_BINARY_DIR}/gl_exports.cpp
    DEPENDS
        ${CMAKE_SOURCE_DIR}/tools/gen_gl_exports.py
        ${CMAKE_SOURCE_DIR}/include/glcompat/frontend/gl_api.hpp
    COMMENT "Generating GL C wrappers for the libEGL shim"
)
set(GL_EXPORTS ${CMAKE_CURRENT_BINARY_DIR}/gl_exports.cpp)

add_library(yaglt_egl_shim SHARED
    egl_shim.cpp
    host_egl.cpp
    ${GL_EXPORTS}
)

# Shipped as libEGL.so so it can replace the system driver on the search path.
set_target_properties(yaglt_egl_shim PROPERTIES OUTPUT_NAME EGL)

target_include_directories(yaglt_egl_shim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(yaglt_egl_shim PRIVATE yaglt_core ${CMAKE_DL_LIBS})

target_link_options(yaglt_egl_shim PRIVATE
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/version.script
)

# The GLES backend is resolved at runtime; no need to link libGLESv2 here.
install(TARGETS yaglt_egl_shim LIBRARY DESTINATION lib)
