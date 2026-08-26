# Core frontend/backend library (foundation phase).
# Backend-specific native implementations (GLES/Vulkan) are added later
# behind the same interfaces; the mock backend is always buildable so the
# headless Linux environment stays self-contained.

add_library(yaglt_core STATIC
    core/capabilities_table.cpp
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

target_compile_features(yaglt_core PUBLIC cxx_std_17)

install(TARGETS yaglt_core ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/glcompat DESTINATION include)
