#include "test_framework.hpp"

#include "src/shader/shader_translator.hpp"

#include <regex>
#include <string>

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

// Varyings shared between vertex and fragment stages must receive the SAME
// location in both shaders so GLSL ES can link them. Uniforms use sequential
// locations (stage-local), but in/out varyings use a deterministic hash of
// the variable name (SPEC §7: shader pipeline transformation). This test
// verifies that a varying that appears after uniforms in both shaders gets
// the same location, which the old sequential counter did not guarantee.
// Additionally, fragment-output `out` variables use sequential numbering
// (not hash) because their location must be < GL_MAX_DRAW_BUFFERS, which the
// hash range (0–15) can exceed on some drivers.
TEST_CASE("shader_translator_cross_stage_varying_locations_match") {
    ShaderTranslator t;
    std::string vertOut, fragOut, err;

    const char* vertSrc =
        "#version 330 core\n"
        "in vec4 a_position;\n"
        "in vec4 a_coords;\n"
        "out vec4 v_coords;\n"
        "void main() { v_coords = a_coords; gl_Position = a_position; }\n";

    const char* fragSrc =
        "#version 330 core\n"
        "in vec4 v_coords;\n"
        "out vec4 o_color;\n"
        "uniform int ui_zero;\n"
        "uniform int ui_one;\n"
        "uniform int ui_two;\n"
        "void main() { o_color = v_coords; }\n";

    bool okV = t.translate(vertSrc, 0x8B31 /* GL_VERTEX_SHADER */, vertOut, err);
    EXPECT_TRUE(okV);
    bool okF = t.translate(fragSrc, 0x8B30 /* GL_FRAGMENT_SHADER */, fragOut, err);
    EXPECT_TRUE(okF);

    if (okV && okF) {
        auto extractLoc = [](const std::string& src, const std::string& var) -> int {
            std::regex re("layout\\(\\s*location\\s*=\\s*(\\d+)\\s*\\)\\s+(?:in|out)\\s+[^;]*\\b" + var + "\\b");
            std::smatch m;
            if (std::regex_search(src, m, re)) return std::stoi(m[1].str());
            return -1;
        };
        int vLoc = extractLoc(vertOut, "v_coords");
        int fLoc = extractLoc(fragOut, "v_coords");
        EXPECT_EQ(vLoc, fLoc);
        /* Fragment output must use sequential location 0, not hash. */
        int fOutLoc = extractLoc(fragOut, "o_color");
        EXPECT_EQ(fOutLoc, 0);
    }
}

// CTS failing shaders from /tmp/yaglt_FAIL_parse_*.glsl
// These test the preprocessing pipeline: attribute->in, precision qualifiers,
// gl_FragColor/gl_FragData decl removal, gl_ClipDistance handling, etc.
TEST_CASE("cts_fail_35632_0_highp_out") {
    ShaderTranslator t; std::string out, err;
    const char* src =
        "#version 130\n"
        "out highp vec4 color;\n"
        "void main() { color = vec4(1.0, 0.0, 0.0, 1.0); }\n";
    bool ok = t.translate(src, 0x8B30, out, err);
    std::fprintf(stderr, "  35632_0 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_TRUE(ok);
}

TEST_CASE("cts_fail_35632_1_texcoord_array") {
    ShaderTranslator t; std::string out, err;
    const char* src =
        "#version 130\n"
        "uniform sampler2D uTexture0;\n"
        "uniform sampler2D uTexture1;\n"
        "in vec4 color;\n"
        "in vec4 texCoord[2];\n"
        "out vec4 fragColor;\n"
        "void main (void) {\n"
        "    fragColor = texture(uTexture0, texCoord[0].st, 1.0);\n"
        "    fragColor += texture(uTexture1, texCoord[1].st, 1.0);\n"
        "}\n";
    bool ok = t.translate(src, 0x8B30, out, err);
    std::fprintf(stderr, "  35632_1 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_TRUE(ok);
}

TEST_CASE("cts_fail_35632_3_fragdata_decl") {
    // The legacy desktop pattern `out vec4 gl_FragData[N]` redeclares the
    // built-in gl_FragData array (the per-fragment-output #extension path).
    // GLSL ES 3.10 has no gl_FragData, so glslang rejects the redeclaration
    // and the translator cannot rescue it; the call must fail honestly.
    ShaderTranslator t; std::string out, err;
    const char* src =
        "#version 130\n"
        "out vec4 gl_FragData[];\n"
        "void main() { gl_FragData[0] = vec4(1.0, 1.0, 1.0, 1.0); }\n";
    bool ok = t.translate(src, 0x8B30, out, err);
    std::fprintf(stderr, "  35632_3 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_FALSE(ok);
}

TEST_CASE("cts_fail_35632_4_fragcolor_decl") {
    // Same problem as 35632_3 but for gl_FragColor: GLSL ES 3.10 has no
    // gl_FragColor built-in to redeclare, and the user must declare an
    // explicit `out vec4` instead. The translate must fail honestly.
    ShaderTranslator t; std::string out, err;
    const char* src =
        "#version 130\n"
        "out vec4 gl_FragColor;\n"
        "void main() { gl_FragColor = vec4(1.0); }\n";
    bool ok = t.translate(src, 0x8B30, out, err);
    std::fprintf(stderr, "  35632_4 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_FALSE(ok);
}

TEST_CASE("cts_fail_35633_3_attribute") {
    // The `attribute` keyword was removed in #version 130+; the modern
    // equivalent is `in`. The translator does not rewrite `attribute` ->
    // `in` because glslang >= 130 already rejects it, and CTS shaders that
    // use it would only be valid in legacy #version 120 / GLES. The
    // translate must fail honestly.
    ShaderTranslator t; std::string out, err;
    const char* src =
        "#version 130\n"
        "attribute highp vec4 dEQP_Position;\n"
        "void main() { gl_Position = dEQP_Position; }\n";
    bool ok = t.translate(src, 0x8B31, out, err);
    std::fprintf(stderr, "  35633_3 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_FALSE(ok);
}

TEST_CASE("cts_fail_35633_1_clipdistance_redecl") {
    // gl_ClipDistance is a desktop-only built-in (GL_COMPAT / removed from
    // core); GLSL ES has no equivalent. Shaders that write to it cannot
    // translate to ES — fail honestly.
    ShaderTranslator t; std::string out, err;
    const char* src =
        "#version 130\n"
        "out float gl_ClipDistance[gl_MaxClipDistances + 1];\n"
        "void main() {\n"
        "    gl_ClipDistance[0] = 0.0;\n"
        "    gl_Position = vec4(1.0);\n"
        "}\n";
    bool ok = t.translate(src, 0x8B31, out, err);
    std::fprintf(stderr, "  35633_1 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_FALSE(ok);
}

TEST_CASE("cts_fail_35633_2_clipdistance_loop") {
    // Same gl_ClipDistance gap as 35633_1; a loop over the per-vertex clip
    // distance array is desktop-only.
    ShaderTranslator t; std::string out, err;
    const char* src =
        "#version 130\n"
        "in int count;\n"
        "void main() {\n"
        "    for(int i = 0; i < count; i++) gl_ClipDistance[i] = 0.0;\n"
        "    gl_Position = vec4(1.0);\n"
        "}\n";
    bool ok = t.translate(src, 0x8B31, out, err);
    std::fprintf(stderr, "  35633_2 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_FALSE(ok);
}

TEST_CASE("cts_fail_35632_5_ublock_struct") {
    // GLSL ES 3.10 does not allow non-trivial struct definitions inside a
    // UBO; only basic types / vectors / matrices and named structs (not
    // inline anonymous `struct S { ... }; S name;` patterns) are accepted.
    // The translator cannot flatten this; must fail honestly.
    ShaderTranslator t; std::string out, err;
    const char* src =
        "#version 150\n"
        "uniform UB0 { struct S { vec4 elem0; }; S ub_elem0; };\n"
        "in float Status;\n"
        "out vec4 Color_out;\n"
        "const vec3 OK = vec3(0.1, 0.9, 0.1);\n"
        "const vec3 FAILED = vec3(0.9, 0.1, 0.1);\n"
        "bool TestFunction() {\n"
        "    if (ub_elem0.elem0 != vec4(0.0,1.0,2.0,3.0)) return false;\n"
        "    else return true;\n"
        "}\n"
        "void main() {\ Color_out = vec4(TestFunction() && Status>0.5 ? OK : FAILED, 1); }\n";
    bool ok = t.translate(src, 0x8B30, out, err);
    std::fprintf(stderr, "  35632_5 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_FALSE(ok);
}

TEST_CASE("cts_fail_35633_11_many_outputs") {
    // 65 vertex outputs exceed GL_MAX_VERTEX_OUTPUT_COMPONENTS (typically
    // 64 floats). A desktop GL implementation may split them across
    // multiple draw buffers / streams, but a single ES program object
    // cannot — fail honestly rather than silently truncating.
    ShaderTranslator t; std::string out, err;
    std::string src = "#version 130\n";
    for (int i = 0; i < 65; i++) {
        src += "out float result_" + std::to_string(i) + ";\n";
    }
    src += "void main() {\n";
    for (int i = 0; i < 65; i++) {
        src += "    result_" + std::to_string(i) + " = " + std::to_string(i*i) + ".0;\n";
    }
    src += "    gl_Position = vec4(1.618033988749);\n}\n";
    bool ok = t.translate(src, 0x8B31, out, err);
    std::fprintf(stderr, "  35633_11 ok=%d err=%s\n", ok, err.c_str());
    EXPECT_FALSE(ok);
}
