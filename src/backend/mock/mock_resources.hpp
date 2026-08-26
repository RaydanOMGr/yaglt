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
    int bufferDataCalls = 0;
    uint32_t lastTarget = 0;
    intptr_t lastSize = 0;
    uint32_t lastUsage = 0;
    bool lastHadData = false;
    void bufferData(uint32_t target, intptr_t size, uint32_t usage,
                    const void* data) override {
        ++bufferDataCalls;
        lastTarget = target;
        lastSize = size;
        lastUsage = usage;
        lastHadData = (data != nullptr);
    }
};
class MockTexture : public BackendTexture {
public:
    int id = 0;
    int texImage2DCalls = 0;
    uint32_t lastTarget = 0;
    int lastLevel = 0;
    uint32_t lastInternalFormat = 0;
    int lastWidth = 0;
    int lastHeight = 0;
    uint32_t lastFormat = 0;
    uint32_t lastType = 0;
    int texParameteriCalls = 0;
    uint32_t lastParamPname = 0;
    int lastParam = 0;
    void texImage2D(uint32_t target, int level, uint32_t internalFormat,
                    int width, int height, uint32_t format, uint32_t type,
                    const void* data) override {
        ++texImage2DCalls;
        lastTarget = target;
        lastLevel = level;
        lastInternalFormat = internalFormat;
        lastWidth = width;
        lastHeight = height;
        lastFormat = format;
        lastType = type;
        (void)data;
    }
    void texParameteri(uint32_t target, uint32_t pname, int param) override {
        ++texParameteriCalls;
        lastTarget = target;
        lastParamPname = pname;
        lastParam = param;
    }
    uint32_t nativeId() const override { return static_cast<uint32_t>(id); }
};
class MockRenderbuffer : public BackendRenderbuffer {
public:
    int id = 0;
    uint32_t nativeId() const override { return static_cast<uint32_t>(id); }
};
class MockFramebuffer : public BackendFramebuffer {
public:
    int id = 0;
    int framebufferTexture2DCalls = 0;
    uint32_t lastTarget = 0;
    uint32_t lastAttachment = 0;
    uint32_t lastTexTarget = 0;
    uint32_t lastNativeTexture = 0;
    int lastLevel = 0;
    int framebufferRenderbufferCalls = 0;
    uint32_t lastRbTarget = 0;
    uint32_t lastNativeRenderbuffer = 0;
    uint32_t checkStatusResult = 0x8CD5; // GL_FRAMEBUFFER_COMPLETE
    void framebufferTexture2D(uint32_t target, uint32_t attachment,
                              uint32_t texTarget, uint32_t nativeTexture,
                              int level) override {
        ++framebufferTexture2DCalls;
        lastTarget = target;
        lastAttachment = attachment;
        lastTexTarget = texTarget;
        lastNativeTexture = nativeTexture;
        lastLevel = level;
    }
    void framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                 uint32_t rbTarget,
                                 uint32_t nativeRenderbuffer) override {
        ++framebufferRenderbufferCalls;
        lastTarget = target;
        lastAttachment = attachment;
        lastRbTarget = rbTarget;
        lastNativeRenderbuffer = nativeRenderbuffer;
    }
    uint32_t checkStatus(uint32_t) const override { return checkStatusResult; }
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
