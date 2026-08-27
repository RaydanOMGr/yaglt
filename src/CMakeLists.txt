# Core frontend/backend library (foundation phase).
# Backend-specific native implementations (GLES/Vulkan) are added later
# behind the same interfaces; the mock backend is always buildable so the
# headless Linux environment stays self-contained.

add_library(yaglt_core STATIC
    core/capabilities_table.cpp
    core/log.cpp
    frontend/context.cpp
    frontend/gl_api.cpp
    state/gl_state.cpp
)

target_include_directories(yaglt_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
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

# Android platform capabilities (SPEC §5/§18). This is portable selection logic
# (SDK-version -> implementation) with no Android header dependencies, so it is
# compiled everywhere and unit-tested on host. The actual native Android API
# calls inside the abstraction are guarded for the device build.
target_sources(yaglt_core PRIVATE
    platform/android/android_capabilities.cpp
    platform/android/android_shared_memory.cpp
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

# Real OpenGL ES backend. Symbols are resolved at runtime via dlopen, so this
# compiles even where libGLESv2 dev libraries are absent; initialize() reports
# honestly when no driver is present.
option(YAGLT_BACKEND_GLES "Build the OpenGL ES backend (runtime-loaded)" ON)
if(YAGLT_BACKEND_GLES)
    target_sources(yaglt_core PRIVATE
        backend/gles/gles_loader.cpp
        backend/gles/gles_capabilities.cpp
        backend/gles/gles_shader_compiler.cpp
        backend/gles/gles_backend.cpp
    )
    target_link_libraries(yaglt_core PUBLIC ${CMAKE_DL_LIBS})
    target_compile_definitions(yaglt_core PUBLIC YAGLT_BACKEND_GLES=1)
endif()

target_compile_features(yaglt_core PUBLIC cxx_std_17)

# Optional: desktop GLSL -> GLSL ES translation via glslang (GLSL->SPIR-V)
# and SPIRV-Cross (SPIR-V->GLSL ES). Builds those libraries from the local
# checkouts at ../glslang-main and ../SPIRV-Cross-main. OFF by default so the
# main build stays self-contained. glslang is built without SPIRV-Tools
# (ENABLE_OPT=OFF).
option(YAGLT_SHADER_TRANSLATE "Build glslang + SPIRV-Cross for shader translation" OFF)
if(YAGLT_SHADER_TRANSLATE)
    set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
    set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
    set(ENABLE_RTTI ON CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_SOURCE_DIR}/../glslang-main
                     ${CMAKE_BINARY_DIR}/glslang_build)
    add_subdirectory(${CMAKE_SOURCE_DIR}/../SPIRV-Cross-main
                     ${CMAKE_BINARY_DIR}/spirv-cross_build)
    target_sources(yaglt_core PRIVATE
        shader/shader_translator.cpp
        backend/gles/gles_translating_compiler.cpp
    )
    target_link_libraries(yaglt_core PUBLIC
        glslang
        glslang-default-resource-limits
        spirv-cross-core
        spirv-cross-glsl
    )
    target_compile_definitions(yaglt_core PUBLIC YAGLT_SHADER_TRANSLATE=1)
endif()

install(TARGETS yaglt_core ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/glcompat DESTINATION include)
