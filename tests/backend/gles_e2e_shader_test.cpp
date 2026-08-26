#include "test_framework.hpp"

#include "glcompat/backend/gles/gles_backend.hpp"

#include <string>

using namespace glcompat;

// End-to-end shader path through the real GLES backend. With no GLES driver
// present (e.g. plain CI) this is skipped and passes vacuously. With a driver
// (Mesa softpipe / Android) it exercises the full desktop GLSL -> glslang ->
// SPIRV-Cross -> GLSL ES -> driver compile pipeline.
TEST_CASE("gles_backend_translates_and_compiles_desktop_vertex_shader") {
    GLESBackend backend;
    if (!backend.initialize()) return;  // no driver: nothing to prove here

    std::string out, err;
    const char* desktopVs =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "void main(){ gl_Position = vec4(aPos, 1.0); }\n";
    bool ok = backend.shaderCompiler().compile(desktopVs, 0x8B31, out, err);
    EXPECT_TRUE(ok);
    if (!ok) std::fprintf(stderr, "  desktop vs error: %s\n", err.c_str());
    EXPECT_NE(out.find("gl_Position"), std::string::npos);

    // Already-GLSL-ES source is passed through without translation.
    const char* esVs =
        "#version 300 es\n"
        "in vec3 aPos;\n"
        "void main(){ gl_Position = vec4(aPos, 1.0); }\n";
    std::string out2, err2;
    EXPECT_TRUE(backend.shaderCompiler().compile(esVs, 0x8B31, out2, err2));

    backend.shutdown();
}

// Desktop uniform-block (UBO) shader must translate to GLSL ES and compile on
// the real driver (SPEC §8: UBO feature mapping behind the shader pipeline).
TEST_CASE("gles_backend_translates_and_compiles_desktop_uniform_block") {
    GLESBackend backend;
    if (!backend.initialize()) return;  // no driver: nothing to prove here

    const char* desktopVs =
        "#version 330 core\n"
        "layout(std140) uniform Matrices {\n"
        "    mat4 uMvp;\n"
        "};\n"
        "layout(location=0) in vec3 aPos;\n"
        "void main(){ gl_Position = uMvp * vec4(aPos, 1.0); }\n";
    std::string out, err;
    bool ok = backend.shaderCompiler().compile(desktopVs, 0x8B31, out, err);
    EXPECT_TRUE(ok);
    if (!ok) std::fprintf(stderr, "  ubo compile error: %s\n", err.c_str());

    backend.shutdown();
}
