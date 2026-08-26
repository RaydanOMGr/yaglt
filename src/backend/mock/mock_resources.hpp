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

    // Uniform recording (observable in tests). Locations and args captured.
    int getUniformLocation(const std::string& name) const override {
        auto it = uniformLocations.find(name);
        if (it != uniformLocations.end()) return it->second;
        int loc = static_cast<int>(uniformLocations.size());
        const_cast<MockProgram*>(this)->uniformLocations[name] = loc;
        return loc;
    }
    int uniform1fCalls = 0, uniform2fCalls = 0, uniform3fCalls = 0,
        uniform4fCalls = 0;
    int uniform1iCalls = 0, uniform2iCalls = 0, uniform3iCalls = 0,
        uniform4iCalls = 0;
    int uniform1fvCalls = 0, uniform1ivCalls = 0, uniformMatrix4fvCalls = 0;
    int lastUniformLoc = -1;
    float lastF0 = 0, lastF1 = 0, lastF2 = 0, lastF3 = 0;
    int lastI0 = 0, lastI1 = 0, lastI2 = 0, lastI3 = 0;
    int lastUniformCount = 0;
    bool lastTranspose = false;
    void uniform1f(int loc, float v0) override {
        ++uniform1fCalls; lastUniformLoc = loc; lastF0 = v0;
    }
    void uniform2f(int loc, float v0, float v1) override {
        ++uniform2fCalls; lastUniformLoc = loc; lastF0 = v0; lastF1 = v1;
    }
    void uniform3f(int loc, float v0, float v1, float v2) override {
        ++uniform3fCalls; lastUniformLoc = loc; lastF0 = v0; lastF1 = v1; lastF2 = v2;
    }
    void uniform4f(int loc, float v0, float v1, float v2, float v3) override {
        ++uniform4fCalls; lastUniformLoc = loc; lastF0 = v0; lastF1 = v1;
        lastF2 = v2; lastF3 = v3;
    }
    void uniform1i(int loc, int v0) override {
        ++uniform1iCalls; lastUniformLoc = loc; lastI0 = v0;
    }
    void uniform2i(int loc, int v0, int v1) override {
        ++uniform2iCalls; lastUniformLoc = loc; lastI0 = v0; lastI1 = v1;
    }
    void uniform3i(int loc, int v0, int v1, int v2) override {
        ++uniform3iCalls; lastUniformLoc = loc; lastI0 = v0; lastI1 = v1; lastI2 = v2;
    }
    void uniform4i(int loc, int v0, int v1, int v2, int v3) override {
        ++uniform4iCalls; lastUniformLoc = loc; lastI0 = v0; lastI1 = v1;
        lastI2 = v2; lastI3 = v3;
    }
    void uniform1fv(int loc, const float* v, int count) override {
        ++uniform1fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) lastF0 = v[0];
    }
    void uniform1iv(int loc, const int* v, int count) override {
        ++uniform1ivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) lastI0 = v[0];
    }
    void uniformMatrix4fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix4fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
    }

    std::unordered_map<std::string, int> attribLocations;
    std::unordered_map<std::string, int> uniformLocations;
};

} // namespace glcompat
