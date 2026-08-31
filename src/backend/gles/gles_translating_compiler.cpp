#include "src/backend/gles/gles_translating_compiler.hpp"

#include "src/core/debug.hpp"

namespace glcompat {

bool TranslatingGLESShaderCompiler::compile(const std::string& source,
                                           uint32_t stage, std::string& output,
                                           std::string& error) {
    // Already GLSL ES: compile directly on the driver.
    if (source.find("#version 300 es") != std::string::npos ||
        source.find("#version 310 es") != std::string::npos ||
        source.find("#version 320 es") != std::string::npos) {
        YAGLT_DEBUG("translating_compiler: ES passthrough (stage=0x%x, %zu bytes)",
                    stage, source.size());
        return es_.compile(source, stage, output, error);
    }

    YAGLT_DEBUG("translating_compiler: desktop translate (stage=0x%x, %zu bytes)",
                stage, source.size());
    std::string es;
    if (!translator_.translate(source, stage, es, error)) {
        return false;
    }
    return es_.compile(es, stage, output, error);
}

} // namespace glcompat
