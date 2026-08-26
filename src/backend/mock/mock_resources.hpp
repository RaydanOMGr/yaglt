#pragma once

#include "glcompat/core/backend_resources.hpp"
#include <string>
#include <unordered_map>

namespace glcompat {

// Mock backend resource handles. They carry only enough bookkeeping to make
// lifetime and creation observable in tests; no native API is involved.
class MockBuffer : public BackendBuffer {
public:
    int id = 0;
};
class MockTexture : public BackendTexture {
public:
    int id = 0;
};
class MockRenderbuffer : public BackendRenderbuffer {
public:
    int id = 0;
};
class MockFramebuffer : public BackendFramebuffer {
public:
    int id = 0;
};
class MockVertexArray : public BackendVertexArray {
public:
    int id = 0;
};
class MockShader : public BackendShader {
public:
    int id = 0;
    bool compile(const std::string& source, std::string& log) override {
        // The mock does not run a real driver; any non-empty source compiles.
        if (source.empty()) {
            log = "empty shader source";
            return false;
        }
        log.clear();
        return true;
    }
};
class MockProgram : public BackendProgram {
public:
    int id = 0;
    std::vector<int> attached; // backend shader ids

    void attach(BackendShader& shader) override {
        if (auto* ms = dynamic_cast<MockShader*>(&shader)) attached.push_back(ms->id);
    }
    bool link(std::string& log) override {
        if (attached.empty()) {
            log = "no shaders attached";
            return false;
        }
        log.clear();
        return true;
    }
    int getAttribLocation(const std::string& name) const override {
        auto it = attribLocations.find(name);
        if (it != attribLocations.end()) return it->second;
        // Assign a stable, deterministic location per name (like a driver would).
        int loc = static_cast<int>(attribLocations.size());
        const_cast<MockProgram*>(this)->attribLocations[name] = loc;
        return loc;
    }
    uint32_t nativeId() const override { return static_cast<uint32_t>(id); }

    std::unordered_map<std::string, int> attribLocations;
};

} // namespace glcompat
