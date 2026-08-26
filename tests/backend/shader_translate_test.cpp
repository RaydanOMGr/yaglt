#include "test_framework.hpp"

#include "src/shader/shader_translator.hpp"

using namespace glcompat;

// Only built/linked when YAGLT_SHADER_TRANSLATE=ON (glslang + SPIRV-Cross).
TEST_CASE("shader_translator_desktop_vertex_to_es") {
    ShaderTranslator t;
    std::string out, err;
    const char* src =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "void main(){ gl_Position = vec4(aPos, 1.0); }\n";
    bool ok = t.translate(src, 0x8B31 /*GL_VERTEX_SHADER*/, out, err);
    EXPECT_TRUE(ok);
    if (ok) {
        EXPECT_NE(out.find("gl_Position"), std::string::npos);
    }
}

// Desktop uniform-block (UBO) shader must translate to GLSL ES with the block
// and its member preserved (SPEC §8: UBO feature mapping behind the pipeline).
TEST_CASE("shader_translator_desktop_uniform_block_to_es") {
    ShaderTranslator t;
    std::string out, err;
    const char* src =
        "#version 330 core\n"
        "layout(std140) uniform Matrices {\n"
        "    mat4 uMvp;\n"
        "};\n"
        "layout(location=0) in vec3 aPos;\n"
        "void main(){ gl_Position = uMvp * vec4(aPos, 1.0); }\n";
    bool ok = t.translate(src, 0x8B31 /*GL_VERTEX_SHADER*/, out, err);
    EXPECT_TRUE(ok);
    if (ok) {
        EXPECT_NE(out.find("uMvp"), std::string::npos);
        EXPECT_NE(out.find("gl_Position"), std::string::npos);
    }
}
