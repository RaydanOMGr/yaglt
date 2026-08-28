#include "src/shader/shader_translator.hpp"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>

#include <regex>
#include <vector>

namespace glcompat {

namespace {
// Desktop GLSL omits explicit `layout(binding=...)` for uniform/storage blocks,
// but glslang requires one when emitting SPIR-V. Inject a default binding for
// every uniform/storage block that lacks one so desktop shaders translate
// without manual edits (SPEC §7: shader pipeline transformation).
std::string assignDefaultBindings(const std::string& src) {
    static const std::regex re(
        R"((layout\s*\([^)]*\)\s*)?(uniform|buffer)\s+([A-Za-z_]\w*)(\s+[A-Za-z_]\w*)?\s*\{)");
    std::string out;
    std::string::const_iterator pos = src.begin();
    int binding = 0;
    std::smatch m;
    while (std::regex_search(pos, src.end(), m, re)) {
        out.append(pos, m[0].first);
        const std::string kind = m[2].str();
        const std::string layout = m[1].str();
        if (layout.empty()) {
            out += "layout(binding=" + std::to_string(binding++) + ") " + kind +
                   " " + m[3].str();
            if (m[4].matched) out += m[4].str();
            out += " {";
        } else if (layout.find("binding") == std::string::npos) {
            const std::string afterOpen = layout.substr(std::string("layout(").size());
            out += "layout(binding=" + std::to_string(binding++) + ", " + afterOpen +
                   kind + " " + m[3].str();
            if (m[4].matched) out += m[4].str();
            out += " {";
        } else {
            out += m[0].str(); // already has a binding
        }
        pos = m[0].second;
    }
    out.append(pos, src.end());
    return out;
}

// User-defined `in`/`out` interface variables and bare `uniform` declarations
// need an explicit location when targeting SPIR-V (glslang rejects unlocated
// user I/O and non-block uniforms). Inject a default location for any such
// declaration that lacks one so desktop GLSL translates without manual edits
// (SPEC §7).
std::string assignDefaultLocations(const std::string& src) {
    static const std::regex re(
        R"((layout\s*\([^)]*\)\s*)?(in|out|uniform)\s+([A-Za-z_]\w*(?:\s*<[^>]*>)?)\s+([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;)");
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

    EShLanguage lang = mapStage(stage);
    glslang::TShader shader(lang);
    std::string prepared = assignDefaultBindings(desktopGlsl);
    prepared = assignDefaultLocations(prepared);
    // Rewrite 1D textures to 2D before parsing: ES has no 1D and modern core
    // GLSL rejects sampler1D / texture1D. This must run on the desktop source.
    prepared = replaceEmulated1D(prepared);
    // Desktop GLSL < 4.20 rejects layout(binding=...) on uniform/storage blocks
    // and layout(location=...) on bare uniforms; these ARB extensions enable them
    // for SPIR-V translation.
    const size_t vpos = prepared.find("#version");
    if (vpos != std::string::npos) {
        const size_t nl = prepared.find('\n', vpos);
        if (nl != std::string::npos) {
            prepared.insert(nl + 1,
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "#extension GL_ARB_explicit_uniform_location : enable\n");
        }
    }
    const char* src = prepared.c_str();
    shader.setStrings(&src, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientOpenGL, 110);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    EShMessages msgs = EShMsgDefault;
    if (!shader.parse(GetDefaultResources(), 110, false, msgs)) {
        error = shader.getInfoLog();
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(msgs)) {
        error = program.getInfoLog();
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
    return true;
}

} // namespace glcompat
