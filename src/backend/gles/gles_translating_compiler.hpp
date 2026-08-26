#pragma once

#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/factory.hpp"
#include "src/backend/gles/gles_shader_compiler.hpp"
#include "src/shader/shader_translator.hpp"

namespace glcompat {

// GLES shader compiler that first translates desktop GLSL to GLSL ES via the
// ShaderTranslator (glslang + SPIRV-Cross) when the input is not already ES,
// then compiles the resulting ES source on the real GLES driver. Built only
// when YAGLT_SHADER_TRANSLATE is enabled.
class TranslatingGLESShaderCompiler : public IShaderCompiler {
public:
    explicit TranslatingGLESShaderCompiler(GLESLibPtr lib)
        : lib_(lib), es_(lib), translator_() {}

    bool compile(const std::string& source, uint32_t stage, std::string& output,
                 std::string& error) override;

private:
    GLESLibPtr lib_;
    GLESShaderCompiler es_;
    ShaderTranslator translator_;
};

} // namespace glcompat
