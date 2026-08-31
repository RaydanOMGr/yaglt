#pragma once

#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/factory.hpp"
#include "src/shader/shader_translator.hpp"

namespace glcompat {

// GLES shader compiler that translates desktop GLSL to GLSL ES via the
// ShaderTranslator (glslang + SPIRV-Cross) when the input is not already ES.
// Built only when YAGLT_SHADER_TRANSLATE is enabled. The actual compilation on
// the driver happens in BackendShader::compile; this class only translates.
class TranslatingGLESShaderCompiler : public IShaderCompiler {
public:
    explicit TranslatingGLESShaderCompiler(GLESLibPtr lib)
        : lib_(lib), translator_() {}

    bool compile(const std::string& source, uint32_t stage, std::string& output,
                 std::string& error) override;

private:
    GLESLibPtr lib_;
    ShaderTranslator translator_;
};

} // namespace glcompat
