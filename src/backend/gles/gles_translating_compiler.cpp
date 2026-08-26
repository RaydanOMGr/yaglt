#include "src/backend/gles/gles_translating_compiler.hpp"

namespace glcompat {

bool TranslatingGLESShaderCompiler::compile(const std::string& source,
                                           uint32_t stage, std::string& output,
                                           std::string& error) {
    // Already GLSL ES: compile directly on the driver.
    if (source.find("#version 300 es") != std::string::npos ||
        source.find("#version 310 es") != std::string::npos ||
        source.find("#version 320 es") != std::string::npos) {
        return es_.compile(source, stage, output, error);
    }

    std::string es;
    if (!translator_.translate(source, stage, es, error)) {
        return false;
    }
    return es_.compile(es, stage, output, error);
}

} // namespace glcompat
