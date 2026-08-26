#pragma once

#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/factory.hpp"

namespace glcompat {

// Real GLES shader compiler. It compiles GLSL ES source on the GLES driver and
// reports the driver's status/log. It does NOT translate desktop GLSL -> GLSL
// ES (that needs glslang, which is not available); desktop inputs will fail at
// the driver and the real error is surfaced. This is honest: no fake success.
class GLESShaderCompiler : public IShaderCompiler {
public:
    explicit GLESShaderCompiler(GLESLibPtr lib) : lib_(lib) {}

    bool compile(const std::string& source, uint32_t stage, std::string& output,
                 std::string& error) override;

private:
    GLESLibPtr lib_;
};

} // namespace glcompat
