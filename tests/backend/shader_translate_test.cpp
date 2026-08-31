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

// 1D textures are emulated on GLES backends as 2D with height=1 (SPEC §7: shader
// pipeline transformation). The translator must rewrite `sampler1D` / `texture1D`
// to the 2D equivalent on the desktop source *before* parsing: modern core GLSL
// rejects `sampler1D` / `texture1D`, so the rewrite must happen up front or the
// shader fails to validate. The 1D coordinate is padded to a 2D coordinate with
// a constant v = 0.5 (the single row of the height=1 emulation surface).
TEST_CASE("shader_translator_1d_emulated_as_2d") {
    ShaderTranslator t;
    std::string out, err;
    const char* src =
        "#version 330 core\n"
        "uniform sampler1D s1d;\n"
        "out vec4 o;\n"
        "void main() { o = texture1D(s1d, 0.5); }\n";
    bool ok = t.translate(src, 0x8B30 /*GL_FRAGMENT_SHADER*/, out, err);
    // The shader must translate successfully (the original `texture1D` would have
    // been rejected by glslang as invalid core GLSL).
    EXPECT_TRUE(ok);
    if (ok) {
        // The 1D types must be gone and replaced by their 2D equivalents.
        EXPECT_EQ(out.find("sampler1D"), std::string::npos);
        EXPECT_EQ(out.find("texture1D"), std::string::npos);
        EXPECT_NE(out.find("sampler2D"), std::string::npos);
    }
}

// OpenGL 3.0 (GLSL 1.30) shaders use legacy built-ins (texture2D / gl_FragColor)
// and a #version below what glslang's OpenGL-SPIR-V path accepts. The translator
// must bump the version and modernize the built-ins so the shader still compiles.
TEST_CASE("shader_translator_legacy_gl30_to_es") {
    ShaderTranslator t;
    std::string out, err;
    const char* src =
        "#version 130\n"
        "uniform sampler2D tex;\n"
        "in vec2 vUv;\n"
        "void main(){ gl_FragColor = texture2D(tex, vUv); }\n";
    bool ok = t.translate(src, 0x8B30 /*GL_FRAGMENT_SHADER*/, out, err);
    EXPECT_TRUE(ok);
    if (ok) {
        EXPECT_EQ(out.find("texture2D"), std::string::npos);
        EXPECT_EQ(out.find("gl_FragColor"), std::string::npos);
        EXPECT_NE(out.find("texture("), std::string::npos);
        EXPECT_NE(out.find("yaglt_FragColor"), std::string::npos);
    } else {
        std::fprintf(stderr, "  legacy gl30 translate error: %s\n", err.c_str());
    }
}

TEST_CASE("tmp_cts_gl30_cover_vert") {
    ShaderTranslator t;
    std::string out, err;
    const char* src =
        "#version 130\n"
        "    out vec3 texCoords;\n"
        "    in vec2 inPosition;\n"
        "    in vec3 inTexCoord;\n"
        "    void main() {\n"
        "        gl_Position = vec4(inPosition.x, inPosition.y, 0.0,1.0);\n"
        "        texCoords = inTexCoord;\n"
        "    }\n";
    bool ok = t.translate(src, 0x8B31, out, err);
    std::fprintf(stderr, "  VERT ok=%d err=%s\n", ok, err.c_str());
    EXPECT_TRUE(ok);
}
TEST_CASE("tmp_cts_gl30_cover_frag") {
    ShaderTranslator t;
    std::string out, err;
    const char* src =
        "#version 130\n"
        "    \n"
        "    uniform sampler2D tex0;\n"
        "    in vec3 texCoords;\n"
        "    out vec4 frag_color;\n"
        "    void main() {\n"
        "        frag_color = texture(tex0, texCoords.xy);\n"
        "    }\n";
    bool ok = t.translate(src, 0x8B30, out, err);
    std::fprintf(stderr, "  FRAG ok=%d err=%s\n", ok, err.c_str());
    EXPECT_TRUE(ok);
}
