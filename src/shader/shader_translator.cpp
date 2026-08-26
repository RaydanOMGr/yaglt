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
    // Desktop GLSL < 4.20 rejects layout(binding=...) on uniform/storage
    // blocks; the 420pack extension enables it for SPIR-V translation.
    const size_t vpos = prepared.find("#version");
    if (vpos != std::string::npos) {
        const size_t nl = prepared.find('\n', vpos);
        if (nl != std::string::npos) {
            prepared.insert(
                nl + 1, "#extension GL_ARB_shading_language_420pack : enable\n");
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
    glsl.set_common_options(opts);

    esSource = glsl.compile();
    return true;
}

} // namespace glcompat
