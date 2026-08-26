#include "src/backend/gles/gles_shader_compiler.hpp"

#include <vector>

namespace glcompat {

bool GLESShaderCompiler::compile(const std::string& source, uint32_t /*stage*/,
                                 std::string& output, std::string& error) {
    output.clear();
    error.clear();

    if (!lib_ || !lib_->loaded) {
        error = "GLES library not loaded";
        return false;
    }
    if (source.empty()) {
        error = "empty shader source";
        return false;
    }

    // Determine stage from the source's #pragma-like markers is out of scope;
    // callers pass a stage via a separate API. For the foundation we compile a
    // vertex/fragment shader based on a simple heuristic so the driver path is
    // exercised. This is intentionally minimal and will reject desktop GLSL.
    GLenum stage = GL_VERTEX_SHADER;
    if (source.find("gl_FragColor") != std::string::npos ||
        source.find("fragment") != std::string::npos) {
        stage = GL_FRAGMENT_SHADER;
    }

    GLuint sh = lib_->glCreateShader(stage);
    if (sh == 0) {
        error = "glCreateShader failed";
        return false;
    }
    const char* src = source.c_str();
    GLint len = static_cast<GLint>(source.size());
    lib_->glShaderSource(sh, 1, &src, &len);
    lib_->glCompileShader(sh);

    GLint ok = 0;
    lib_->glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        GLint logLen = 0;
        lib_->glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> buf(logLen > 0 ? logLen : 1, 0);
        lib_->glGetShaderInfoLog(sh, logLen, nullptr, buf.data());
        error = std::string(buf.data());
        lib_->glDeleteShader(sh);
        return false;
    }

    // Pass-through: the compiled shader lives in the driver; we return the
    // original source as the "translated" output for now (no transformation).
    output = source;
    lib_->glDeleteShader(sh);
    return true;
}

} // namespace glcompat
