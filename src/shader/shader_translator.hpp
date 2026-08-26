#pragma once

#include <cstdint>
#include <string>

namespace glcompat {

// Translates desktop OpenGL GLSL into OpenGL ES GLSL so the GLES backend can
// consume it. Pipeline: glslang (GLSL -> SPIR-V) -> SPIRV-Cross (SPIR-V ->
// GLSL ES). Only built when YAGLT_SHADER_TRANSLATE is enabled (the glslang and
// SPIRV-Cross libraries must be available). Without it, shader translation is
// unavailable and the GLES backend only accepts GLSL ES input directly.
class ShaderTranslator {
public:
    ShaderTranslator();
    ~ShaderTranslator();

    // stage: GL_VERTEX_SHADER / GL_FRAGMENT_SHADER / GL_COMPUTE_SHADER / etc.
    // Returns true and fills `esSource` with GLSL ES, or false with `error`.
    bool translate(const std::string& desktopGlsl, uint32_t stage,
                   std::string& esSource, std::string& error);

private:
    bool initialized_ = false;
};

} // namespace glcompat
