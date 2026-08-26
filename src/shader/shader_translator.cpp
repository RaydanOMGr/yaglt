#include "src/shader/shader_translator.hpp"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>

#include <vector>

namespace glcompat {

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
    const char* src = desktopGlsl.c_str();
    shader.setStrings(&src, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientOpenGL, 110);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    if (!shader.parse(GetDefaultResources(), 110, false,
                      EShMsgDefault)) {
        error = shader.getInfoLog();
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault)) {
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
