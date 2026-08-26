#pragma once

#include "glcompat/core/factory.hpp"

namespace glcompat {

// Test shader compiler. In the mock backend, compilation is a pass-through:
// source is accepted unchanged. This lets the abstraction be exercised
// without a real GLSL toolchain in headless tests.
class MockShaderCompiler : public IShaderCompiler {
public:
    bool compile(const std::string& source,
                 uint32_t /*stage*/,
                 std::string& output,
                 std::string& error) override {
        if (source.empty()) {
            error = "empty shader source";
            return false;
        }
        output = source;
        error.clear();
        return true;
    }
};

} // namespace glcompat
