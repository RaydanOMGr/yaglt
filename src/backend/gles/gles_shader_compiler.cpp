#include "src/backend/gles/gles_shader_compiler.hpp"

#include <vector>

namespace glcompat {
bool GLESShaderCompiler::compile(const std::string& source,
                                 uint32_t /*stage*/,
                                 std::string& output,
                                 std::string& error) {
    // This IShaderCompiler implementation is a pass-through for GLSL ES source.
    // The actual compilation (glCreateShader/glShaderSource/glCompileShader) is
    // performed by GLESBackendShader::compile on the driver-owned shader object,
    // which was created with the correct stage when the frontend called
    // createShader(stage). Doing driver compilation here would require guessing
    // the stage from the source text (a fragile heuristic), so it is avoided.
    output.clear();
    error.clear();
    if (source.empty()) {
        error = "empty shader source";
        return false;
    }
    output = source;
    return true;
}

} // namespace glcompat
