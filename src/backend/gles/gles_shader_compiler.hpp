#pragma once

#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/factory.hpp"

namespace glcompat {

// Pass-through IShaderCompiler for GLSL ES source. Used when
// YAGLT_SHADER_TRANSLATE is OFF: the frontend source must already be valid
// GLSL ES (or desktop GLSL will fail downstream in the driver). The actual
// driver compilation happens in BackendShader::compile.
class GLESShaderCompiler : public IShaderCompiler {
public:
    explicit GLESShaderCompiler(GLESLibPtr lib) : lib_(lib) {}

    bool compile(const std::string& source, uint32_t stage, std::string& output,
                 std::string& error) override;

private:
    GLESLibPtr lib_;
};

} // namespace glcompat
