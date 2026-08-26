# Core frontend/backend library (foundation phase).
# Backend-specific native implementations (GLES/Vulkan) are added later
# behind the same interfaces; the mock backend is always buildable so the
# headless Linux environment stays self-contained.

add_library(yaglt_core STATIC
    core/capabilities_table.cpp
    frontend/context.cpp
    frontend/gl_api.cpp
)

target_include_directories(yaglt_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Mock backend (header-only resources/factory/compiler; no native deps).
target_sources(yaglt_core PRIVATE
    backend/mock/mock_resources.hpp
    backend/mock/mock_factory.hpp
    backend/mock/mock_shader_compiler.hpp
    backend/mock/mock_capabilities.hpp
    backend/mock/mock_backend.hpp
)

# Linux platform capabilities (real impl for headless development).
target_sources(yaglt_core PRIVATE
    platform/linux/linux_capabilities.hpp
)

# Optional: build against Mesa's android_stub headers on a non-Android host to
# exercise the Android platform path (SPEC §5/§18). The stub needs the compat
# shim (force-included) because it uses NDK nullability/availability macros.
option(YAGLT_ANDROID_STUB "Build against Mesa android_stub headers on host" OFF)
if(YAGLT_ANDROID_STUB)
    target_include_directories(yaglt_core PUBLIC ${CMAKE_SOURCE_DIR}/include/android_stub)
    target_compile_definitions(yaglt_core PUBLIC YAGLT_ANDROID_STUB=1)
    target_compile_options(yaglt_core PUBLIC
        -include ${CMAKE_SOURCE_DIR}/include/android_stub/compat.h)
    target_sources(yaglt_core PRIVATE
        platform/android/android_stub_check.cpp
    )
endif()

target_compile_features(yaglt_core PUBLIC cxx_std_17)

install(TARGETS yaglt_core ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/glcompat DESTINATION include)
