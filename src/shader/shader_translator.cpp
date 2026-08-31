#include "src/shader/shader_translator.hpp"

#include "src/core/debug.hpp"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>

#include <cstdio>
#include <cstdlib>
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace glcompat {

namespace {
// Desktop GLSL omits explicit `layout(binding=...)` for uniform/storage blocks
// and for bare (non-block) uniform declarations, but glslang requires a binding
// when emitting SPIR-V. Inject a default binding for every uniform/storage
// block and bare uniform that lacks one so desktop shaders translate without
// manual edits (SPEC §7: shader pipeline transformation).
std::string assignDefaultBindings(const std::string& src) {
    static const std::regex re(
        // block form:  uniform|buffer NAME [instance] {
        // bare form:   uniform|buffer TYPE NAME [array] ;
        R"((layout\s*\([^)]*\)\s*)?(uniform|buffer)\s+)"
        R"((?:([A-Za-z_]\w*)(?:\s+([A-Za-z_]\w*))?\s*\{)"
        R"(|([A-Za-z_]\w*(?:\s*<[^>]*>)?)\s+([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;))");
    std::string out;
    std::string::const_iterator pos = src.begin();
    int binding = 0;
    std::smatch m;
    while (std::regex_search(pos, src.end(), m, re)) {
        out.append(pos, m[0].first);
        const std::string kind = m[2].str();
        const std::string layout = m[1].str();
        if (m[3].matched) {
            // Block declaration: NAME [instance] {.
            const std::string blockName = m[3].str();
            const std::string instName = m[4].str();
            if (layout.empty()) {
                out += "layout(binding=" + std::to_string(binding++) + ") " + kind +
                       " " + blockName;
                if (!instName.empty()) out += " " + instName;
                out += " {";
            } else if (layout.find("binding") == std::string::npos) {
                const std::string afterOpen =
                    layout.substr(std::string("layout(").size());
                out += "layout(binding=" + std::to_string(binding++) + ", " +
                       afterOpen + kind + " " + blockName;
                if (!instName.empty()) out += " " + instName;
                out += " {";
            } else {
                out += m[0].str(); // already has a binding
            }
        } else {
            // Bare uniform: TYPE NAME [array] ;.
            const std::string type = m[5].str();
            const std::string name = m[6].str();
            const std::string arr = m[7].str();
            if (layout.empty()) {
                out += "layout(binding=" + std::to_string(binding++) + ") " + kind +
                       " " + type + " " + name + arr + ";";
            } else if (layout.find("binding") == std::string::npos) {
                const std::string afterOpen =
                    layout.substr(std::string("layout(").size());
                out += "layout(binding=" + std::to_string(binding++) + ", " +
                       afterOpen + kind + " " + type + " " + name + arr + ";";
            } else {
                out += m[0].str(); // already has a binding
            }
        }
        pos = m[0].second;
    }
    out.append(pos, src.end());
    return out;
}

// User-defined `in`/`out` interface variables need an explicit location when
// targeting SPIR-V (glslang rejects unlocated user I/O). `uniform` declarations
// get a *binding* instead (see assignDefaultBindings) — never a location, which
// SPIR-V rejects. Inject a default location for any `in`/`out` that lacks one so
// desktop GLSL translates without manual edits (SPEC §7).
std::string assignDefaultLocations(const std::string& src) {
    static const std::regex re(
        R"((layout\s*\([^)]*\)\s*)?(in|out)\s+([A-Za-z_]\w*(?:\s*<[^>]*>)?)\s+([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;)");
    std::string out;
    std::string::const_iterator pos = src.begin();
    int location = 0;
    std::smatch m;
    while (std::regex_search(pos, src.end(), m, re)) {
        out.append(pos, m[0].first);
        const std::string layout = m[1].str();
        const std::string qual = m[2].str();
        const std::string type = m[3].str();
        const std::string name = m[4].str();
        const std::string arr = m[5].str();
        if (layout.empty()) {
            out += "layout(location=" + std::to_string(location++) + ") " + qual +
                   " " + type + " " + name + arr + ";";
        } else if (layout.find("location") == std::string::npos) {
            const std::string afterOpen =
                layout.substr(std::string("layout(").size());
            out += "layout(location=" + std::to_string(location++) + ", " +
                    afterOpen + qual + " " + type + " " + name + arr + ";";
        } else {
            out += m[0].str();
        }
        pos = m[0].second;
    }
    out.append(pos, src.end());
    return out;
}
} // namespace

namespace {
// OpenGL ES has no 1D textures. Desktop GLSL using `sampler1D` / `texture1D`
// must be rewritten to the 2D equivalent *before* glslang parses it, because
// `texture1D` / `sampler1D` are absent from modern core GLSL and would fail
// validation. The 1D coordinate is padded to a 2D coordinate with a constant
// v = 0.5 (the row of the height=1 emulation surface). This mirrors the GLES
// backend emulating 1D storage as 2D with height=1 (SPEC §7: shader pipeline
// transformation).
std::string replaceEmulated1D(std::string src) {
    // Sampler type: 1D -> 2D (covers sampler1D / sampler1DShadow / sampler1DArray).
    src = std::regex_replace(src, std::regex(R"(sampler1D\b)"), "sampler2D");
    // Fetch functions: texture1D(s, x[, bias]) -> texture(s, vec2(x, 0.5)[, bias]).
    // Handle the optional 3rd bias argument so the rewrite stays valid.
    std::regex texRe(
        R"(texture1D\s*\(\s*([^,]+?)\s*,\s*([^,]+?)\s*(?:,\s*([^)]+?)\s*)?\))");
    std::string result;
    std::string::const_iterator pos = src.cbegin();
    std::smatch m;
    while (std::regex_search(pos, src.cend(), m, texRe)) {
        result.append(pos, m[0].first);
        result += "texture(" + m[1].str() + ", vec2(" + m[2].str() + ", 0.5)";
        if (m[3].matched) result += ", " + m[3].str();
        result += ")";
        pos = m[0].second;
    }
    result.append(pos, src.cend());
    return result;
}
} // namespace

namespace {
// Legacy (pre-GLSL-330) desktop GLSL uses texture/fragment-output built-ins
// that glslang's OpenGL-SPIR-V path rejects, and that no longer exist once a
// shader is bumped to a modern #version. Rewrite them to their modern forms:
//   texture2D/texture3D/textureCube/shadow2D   -> texture
//   *Lod / *Proj variants                        -> textureLod / textureProj
//   gl_FragColor                                 -> out vec4 yaglt_FragColor
//   gl_FragData[i]                               -> out vec4 yaglt_FragData_i
// `outDecls` collects the global `out` declarations the output rewrites need,
// which the caller inserts just after the #version line (SPEC §7).
std::string modernizeLegacyBuiltins(const std::string& src, std::string& outDecls) {
    outDecls.clear();
    std::string out = src;

    // Texture lookups: longest names first so shorter prefixes don't clobber
    // them (e.g. texture2DLod before texture2D).
    out = std::regex_replace(out, std::regex(R"(\btexture2DLod\s*\()"), "textureLod(");
    out = std::regex_replace(out, std::regex(R"(\btexture3DLod\s*\()"), "textureLod(");
    out = std::regex_replace(out, std::regex(R"(\btextureCubeLod\s*\()"), "textureLod(");
    out = std::regex_replace(out, std::regex(R"(\btexture2DProj\s*\()"), "textureProj(");
    out = std::regex_replace(out, std::regex(R"(\btexture3DProj\s*\()"), "textureProj(");
    out = std::regex_replace(out, std::regex(R"(\bshadow2DProj\s*\()"), "textureProj(");
    out = std::regex_replace(out, std::regex(R"(\btexture2D\s*\()"), "texture(");
    out = std::regex_replace(out, std::regex(R"(\btexture3D\s*\()"), "texture(");
    out = std::regex_replace(out, std::regex(R"(\btextureCube\s*\()"), "texture(");
    out = std::regex_replace(out, std::regex(R"(\bshadow2D\s*\()"), "texture(");

    if (out.find("gl_FragColor") != std::string::npos) {
        out = std::regex_replace(out, std::regex(R"(\bgl_FragColor\b)"),
                                 "yaglt_FragColor");
        // Fragment outputs are user I/O in SPIR-V and require an explicit
        // location, just like any other `out` variable.
        outDecls += "layout(location=0) out vec4 yaglt_FragColor;\n";
    }

    std::regex fdRe(R"(\bgl_FragData\s*\[\s*(\d+)\s*\])");
    std::set<int> fdIndices;
    std::string::const_iterator pos = out.cbegin();
    std::smatch m;
    while (std::regex_search(pos, out.cend(), m, fdRe)) {
        fdIndices.insert(std::atoi(m[1].str().c_str()));
        pos = m[0].second;
    }
    for (int i : fdIndices) {
        char buf[64];
        std::snprintf(buf, sizeof(buf),
                      "layout(location=%d) out vec4 yaglt_FragData_%d;\n", i, i);
        outDecls += buf;
        std::regex repl("gl_FragData\\s*\\[\\s*" + std::to_string(i) +
                        "\\s*\\]");
        out = std::regex_replace(out, repl, "yaglt_FragData_" + std::to_string(i));
    }
    return out;
}

// glslang's OpenGL-SPIR-V path requires desktop GLSL >= 330. OpenGL 3.0/3.1/3.2
// shaders use #version 130/140/150, which would be rejected. Bump such legacy
// desktop shaders to #version 450 (matching EShTargetOpenGL_450, where
// layout(binding=)/layout(location=) are core). GLSL ES inputs are passed
// through untouched. `outDecls` (from modernizeLegacyBuiltins) is inserted just
// after the #version line.
std::string bumpDesktopVersion(std::string src, const std::string& outDecls) {
    static const std::regex verRe(R"(#version[ \t]+(\d+)(?:[ \t]+([a-zA-Z]+))?)");
    std::smatch m;
    if (!std::regex_search(src, m, verRe)) {
        std::string injected = "#version 450\n" + outDecls;
        return injected + src;
    }
    const int ver = std::atoi(m[1].str().c_str());
    const std::string profile = m[2].str();
    if (profile == "es") return src; // already GLSL ES

    std::string result = src;
    if (ver < 450) {
        result = std::regex_replace(result, verRe, "#version 450");
    }
    if (!outDecls.empty()) {
        const size_t vpos = result.find("#version 450");
        const size_t nl = result.find('\n', vpos);
        if (nl != std::string::npos) result.insert(nl + 1, outDecls);
    }
    return result;
}
} // namespace

namespace {
EShLanguage mapStage(uint32_t stage) {
    switch (stage) {
    case 0x8B31: return EShLangVertex;       // GL_VERTEX_SHADER
    case 0x8B30: return EShLangFragment;     // GL_FRAGMENT_SHADER
    case 0x8B32: return EShLangGeometry;     // GL_GEOMETRY_SHADER
    case 0x8DD9: return EShLangTessControl;  // GL_TESS_CONTROL_SHADER
    case 0x8E88: return EShLangTessEvaluation;// GL_TESS_EVALUATION_SHADER
    case 0x91B9: return EShLangCompute;      // GL_COMPUTE_SHADER
    default:     return EShLangVertex;
    }
}
} // namespace

ShaderTranslator::ShaderTranslator() {
    initialized_ = glslang::InitializeProcess();
}

ShaderTranslator::~ShaderTranslator() {
    if (initialized_) glslang::FinalizeProcess();
}

bool ShaderTranslator::translate(const std::string& desktopGlsl, uint32_t stage,
                                 std::string& esSource, std::string& error) {
    esSource.clear();
    error.clear();
    if (!initialized_) {
        error = "glslang not initialized";
        return false;
    }

    {
        std::string _p = "/tmp/yaglt_desktop_in_" + std::to_string(stage) + ".glsl";
        FILE* _f = std::fopen(_p.c_str(), "wb");
        if (_f) { std::fwrite(desktopGlsl.data(), 1, desktopGlsl.size(), _f); std::fclose(_f); }
    }
    YAGLT_DEBUG_DUMP(("desktop_in_" + std::to_string(stage) + ".glsl").c_str(),
                      desktopGlsl);

    EShLanguage lang = mapStage(stage);
    glslang::TShader shader(lang);
    std::string prepared = assignDefaultBindings(desktopGlsl);
    prepared = assignDefaultLocations(prepared);
    // Rewrite legacy desktop built-ins (texture2D/texture3D/textureCube,
    // gl_FragColor/gl_FragData) into their modern equivalents. glslang rejects
    // these for SPIR-V output, and they no longer exist in the core GLSL
    // versions we bump shaders to (SPEC §7: shader pipeline transformation).
    std::string fragDecls;
    prepared = modernizeLegacyBuiltins(prepared, fragDecls);
    // Rewrite 1D textures to 2D before parsing: ES has no 1D and modern core
    // GLSL rejects sampler1D / texture1D. This must run on the desktop source.
    prepared = replaceEmulated1D(prepared);
    // glslang requires desktop GLSL >= 330 to emit SPIR-V. OpenGL 3.0/3.1/3.2
    // shaders use #version 130/140/150 and would be rejected; bump them to 450
    // (matching EShTargetOpenGL_450, where layout(binding=)/layout(location=)
    // are core) and inject any collected fragment-output declarations.
    prepared = bumpDesktopVersion(prepared, fragDecls);

    {
        std::string _p = "/tmp/yaglt_prepared_" + std::to_string(stage) + ".glsl";
        FILE* _f = std::fopen(_p.c_str(), "wb");
        if (_f) { std::fwrite(prepared.data(), 1, prepared.size(), _f); std::fclose(_f); }
    }
    YAGLT_DEBUG_DUMP(("prepared_" + std::to_string(stage) + ".glsl").c_str(),
                      prepared);

    const char* src = prepared.c_str();
    shader.setStrings(&src, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientOpenGL, 450);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    EShMessages msgs = EShMsgDefault;
    if (!shader.parse(GetDefaultResources(), 450, false, msgs)) {
        error = shader.getInfoLog();
        YAGLT_DEBUG("translate: parse failed (stage=0x%x): %s", stage,
                    error.c_str());
        {
            static int n = 0;
            std::string p = "/tmp/yaglt_FAIL_parse_" + std::to_string(stage) +
                            "_" + std::to_string(n++) + ".glsl";
            FILE* f = std::fopen(p.c_str(), "wb");
            if (f) { std::fwrite(desktopGlsl.data(), 1, desktopGlsl.size(), f);
                     std::fclose(f); }
        }
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(msgs)) {
        error = program.getInfoLog();
        static int n = 0;
        std::string p = "/tmp/yaglt_FAIL_link_" + std::to_string(stage) +
                        "_" + std::to_string(n++) + ".glsl";
        FILE* f = std::fopen(p.c_str(), "wb");
        if (f) { std::fwrite(desktopGlsl.data(), 1, desktopGlsl.size(), f);
                 std::fclose(f); }
        return false;
    }

    glslang::TIntermediate* interm = program.getIntermediate(lang);
    if (interm == nullptr) {
        error = "failed to get shader intermediate";
        return false;
    }

    std::vector<unsigned int> spirv;
    glslang::GlslangToSpv(*interm, spirv);

    spirv_cross::CompilerGLSL glsl(spirv.data(), spirv.size());
    spirv_cross::CompilerGLSL::Options opts = glsl.get_common_options();
    opts.version = 310;      // target GLSL ES 3.10
    opts.es = true;
    opts.vulkan_semantics = false;
    // GLSL ES requires explicit precision for float in fragment shaders; emit a
    // default highp precision so translated desktop uniforms compile (SPEC §7).
    opts.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Highp;
    opts.fragment.default_int_precision = spirv_cross::CompilerGLSL::Options::Highp;
    glsl.set_common_options(opts);

    esSource = glsl.compile();
    {
        std::string _p = "/tmp/yaglt_es_out_" + std::to_string(stage) + ".glsl";
        FILE* _f = std::fopen(_p.c_str(), "wb");
        if (_f) { std::fwrite(esSource.data(), 1, esSource.size(), _f); std::fclose(_f); }
    }
    YAGLT_DEBUG_DUMP(("es_out_" + std::to_string(stage) + ".glsl").c_str(),
                      esSource);
    return true;
}

} // namespace glcompat
