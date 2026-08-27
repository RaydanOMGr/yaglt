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
    int bufferSubDataCalls = 0;
    intptr_t lastSubOffset = 0;
    intptr_t lastSubSize = 0;
    bool lastSubHadData = false;
    void bufferSubData(uint32_t target, intptr_t offset, intptr_t size,
                       const void* data) override {
        ++bufferSubDataCalls;
        lastTarget = target;
        lastSubOffset = offset;
        lastSubSize = size;
        lastSubHadData = (data != nullptr);
    }
    int bufferStorageCalls = 0;
    intptr_t lastStorageSize = 0;
    uint32_t lastStorageFlags = 0;
    bool lastStorageHadData = false;
    void bufferStorage(uint32_t target, intptr_t size, uint32_t flags,
                       const void* data) override {
        ++bufferStorageCalls;
        lastTarget = target;
        lastStorageSize = size;
        lastStorageFlags = flags;
        lastStorageHadData = (data != nullptr);
    }
    int copySubDataCalls = 0;
    uint32_t lastCopyReadTarget = 0;
    uint32_t lastCopyWriteTarget = 0;
    intptr_t lastCopyReadOffset = 0;
    intptr_t lastCopyWriteOffset = 0;
    intptr_t lastCopySize = 0;
    void copySubData(uint32_t readTarget, uint32_t writeTarget,
                     intptr_t readOffset, intptr_t writeOffset,
                     intptr_t size) override {
        ++copySubDataCalls;
        lastCopyReadTarget = readTarget;
        lastCopyWriteTarget = writeTarget;
        lastCopyReadOffset = readOffset;
        lastCopyWriteOffset = writeOffset;
        lastCopySize = size;
    }
    int mapBufferRangeCalls = 0;
    intptr_t lastMapOffset = 0;
    intptr_t lastMapLength = 0;
    uint32_t lastMapAccess = 0;
    int unmapBufferCalls = 0;
    void* mapBufferRange(uint32_t target, intptr_t offset, intptr_t length,
                          uint32_t access) override {
        ++mapBufferRangeCalls;
        lastTarget = target;
        lastMapOffset = offset;
        lastMapLength = length;
        lastMapAccess = access;
        return nullptr; // frontend serves the CPU mirror
    }
    void unmapBuffer(uint32_t target) override {
        ++unmapBufferCalls;
        lastTarget = target;
    }
    int invalidateBufferDataCalls = 0;
    int invalidateBufferSubDataCalls = 0;
    uint32_t lastInvalidateTarget = 0;
    intptr_t lastInvalidateOffset = 0;
    intptr_t lastInvalidateLength = 0;
    void invalidateBufferData(uint32_t target) override {
        ++invalidateBufferDataCalls;
        lastInvalidateTarget = target;
    }
    void invalidateBufferSubData(uint32_t target, intptr_t offset,
                                 intptr_t length) override {
        ++invalidateBufferSubDataCalls;
        lastInvalidateTarget = target;
        lastInvalidateOffset = offset;
        lastInvalidateLength = length;
    }
};
class MockTexture : public BackendTexture {
public:
    int id = 0;
    int texImage2DCalls = 0;
    int texImage1DCalls = 0;
    int texImage3DCalls = 0;
    uint32_t last1DTarget = 0;
    int last1DLevel = 0;
    uint32_t last1DInternalFormat = 0;
    int last1DWidth = 0;
    uint32_t last1DFormat = 0;
    uint32_t last1DType = 0;
    uint32_t last3DTarget = 0;
    int last3DLevel = 0;
    uint32_t last3DInternalFormat = 0;
    int last3DWidth = 0;
    int last3DHeight = 0;
    int last3DDepth = 0;
    uint32_t last3DFormat = 0;
    uint32_t last3DType = 0;
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
    void texImage1D(uint32_t target, int level, uint32_t internalFormat,
                    int width, uint32_t format, uint32_t type,
                    const void* data) override {
        ++texImage1DCalls;
        last1DTarget = target;
        last1DLevel = level;
        last1DInternalFormat = internalFormat;
        last1DWidth = width;
        last1DFormat = format;
        last1DType = type;
        (void)data;
    }
    void texImage3D(uint32_t target, int level, uint32_t internalFormat,
                    int width, int height, int depth, uint32_t format,
                    uint32_t type, const void* data) override {
        ++texImage3DCalls;
        last3DTarget = target;
        last3DLevel = level;
        last3DInternalFormat = internalFormat;
        last3DWidth = width;
        last3DHeight = height;
        last3DDepth = depth;
        last3DFormat = format;
        last3DType = type;
        (void)data;
    }
    void texParameteri(uint32_t target, uint32_t pname, int param) override {
        ++texParameteriCalls;
        lastTarget = target;
        lastParamPname = pname;
        lastParam = param;
    }
    int texParameterfCalls = 0;
    float lastParamf = 0.0f;
    int texParameterfvCalls = 0;
    int texParameterivCalls = 0;
    std::vector<float> lastParamfv;
    std::vector<int> lastParamiv;
    void texParameterf(uint32_t target, uint32_t pname, float param) override {
        ++texParameterfCalls;
        lastTarget = target;
        lastParamPname = pname;
        lastParamf = param;
    }
    void texParameterfv(uint32_t target, uint32_t pname, const float* params,
                        int count) override {
        ++texParameterfvCalls;
        lastTarget = target;
        lastParamPname = pname;
        lastParamfv.assign(params, params + count);
    }
    void texParameteriv(uint32_t target, uint32_t pname, const int* params,
                        int count) override {
        ++texParameterivCalls;
        lastTarget = target;
        lastParamPname = pname;
        lastParamiv.assign(params, params + count);
    }
    int texParameterIivCalls = 0;
    int texParameterIuivCalls = 0;
    std::vector<int32_t> lastParamIiv;
    std::vector<uint32_t> lastParamIuiv;
    void texParameterIiv(uint32_t target, uint32_t pname, const int32_t* params,
                        int count) override {
        ++texParameterIivCalls;
        lastTarget = target;
        lastParamPname = pname;
        lastParamIiv.assign(params, params + count);
    }
    void texParameterIuiv(uint32_t target, uint32_t pname, const uint32_t* params,
                         int count) override {
        ++texParameterIuivCalls;
        lastTarget = target;
        lastParamPname = pname;
        lastParamIuiv.assign(params, params + count);
    }
    int invalidateTexImageCalls = 0;
    int invalidateTexSubImageCalls = 0;
    int lastInvLevel = 0;
    int lastInvX = 0, lastInvY = 0, lastInvZ = 0, lastInvW = 0, lastInvH = 0,
        lastInvD = 0;
    void invalidateTexImage(uint32_t target, int level) override {
        ++invalidateTexImageCalls;
        lastTarget = target;
        lastInvLevel = level;
    }
    void invalidateTexSubImage(uint32_t target, int level, int xoffset, int yoffset,
                             int zoffset, int width, int height, int depth) override {
        ++invalidateTexSubImageCalls;
        lastTarget = target;
        lastInvLevel = level;
        lastInvX = xoffset; lastInvY = yoffset; lastInvZ = zoffset;
        lastInvW = width; lastInvH = height; lastInvD = depth;
    }
    int texSubImage1DCalls = 0;
    int texSubImage2DCalls = 0;
    int texSubImage3DCalls = 0;
    uint32_t lastSubTarget = 0;
    int lastSubLevel = 0;
    int lastSubXoffset = 0, lastSubYoffset = 0, lastSubZoffset = 0;
    int lastSubWidth = 0, lastSubHeight = 0, lastSubDepth = 0;
    uint32_t lastSubFormat = 0, lastSubType = 0;
    int copyTexImage1DCalls = 0;
    int copyTexImage2DCalls = 0;
    uint32_t lastCopyInternalFormat = 0;
    int lastCopyX = 0, lastCopyY = 0;
    int lastCopyWidth = 0, lastCopyHeight = 0, lastCopyBorder = 0;
    void texSubImage1D(uint32_t target, int level, int xoffset, int width,
                       uint32_t format, uint32_t type, const void* data) override {
        ++texSubImage1DCalls;
        lastSubTarget = target; lastSubLevel = level; lastSubXoffset = xoffset;
        lastSubWidth = width; lastSubFormat = format; lastSubType = type;
        (void)data;
    }
    void texSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                       int width, int height, uint32_t format, uint32_t type,
                       const void* data) override {
        ++texSubImage2DCalls;
        lastSubTarget = target; lastSubLevel = level; lastSubXoffset = xoffset;
        lastSubYoffset = yoffset; lastSubWidth = width; lastSubHeight = height;
        lastSubFormat = format; lastSubType = type;
        (void)data;
    }
    void texSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                       int zoffset, int width, int height, int depth,
                       uint32_t format, uint32_t type, const void* data) override {
        ++texSubImage3DCalls;
        lastSubTarget = target; lastSubLevel = level; lastSubXoffset = xoffset;
        lastSubYoffset = yoffset; lastSubZoffset = zoffset; lastSubWidth = width;
        lastSubHeight = height; lastSubDepth = depth; lastSubFormat = format;
        lastSubType = type;
        (void)data;
    }
    void copyTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                        int x, int y, int width, int border) override {
        ++copyTexImage1DCalls;
        lastSubTarget = target; lastSubLevel = level;
        lastCopyInternalFormat = internalFormat; lastCopyX = x; lastCopyY = y;
        lastCopyWidth = width; lastCopyBorder = border;
    }
    void copyTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                         int x, int y, int width, int height, int border) override {
        ++copyTexImage2DCalls;
        lastSubTarget = target; lastSubLevel = level;
        lastCopyInternalFormat = internalFormat; lastCopyX = x; lastCopyY = y;
        lastCopyWidth = width; lastCopyHeight = height; lastCopyBorder = border;
    }
    int storage1DCalls = 0, storage2DCalls = 0, storage3DCalls = 0;
    int generateMipmapCalls = 0;
    int textureBufferCalls = 0, textureBufferRangeCalls = 0;
    int getTexImageCalls = 0, getLevelParameterCalls = 0;
    uint32_t lastStorageTarget = 0, lastStorageInternalFormat = 0;
    uint32_t lastBufferInternalFormat = 0;
    uint32_t lastBufferNativeId = 0;
    intptr_t lastBufferOffset = 0, lastBufferSize = 0;
    int lastStorageLevels = 0, lastStorageW = 0, lastStorageH = 0, lastStorageD = 0;
    int lastLevelParamLevel = 0;
    uint32_t lastLevelParamPname = 0;
    int storage2DMultisampleCalls = 0, storage3DMultisampleCalls = 0;
    int texImage2DMultisampleCalls = 0, texImage3DMultisampleCalls = 0;
    int lastMSamples = 0, lastMWidth = 0, lastMHeight = 0, lastMDepth = 0;
    bool lastMFixed = false;
    uint32_t lastMTarget = 0, lastMInternalFormat = 0;
    void storage1D(uint32_t target, int levels, uint32_t internalFormat,
                   int width) override {
        ++storage1DCalls; lastStorageTarget = target; lastStorageLevels = levels;
        lastStorageInternalFormat = internalFormat; lastStorageW = width;
    }
    void storage2D(uint32_t target, int levels, uint32_t internalFormat,
                   int width, int height) override {
        ++storage2DCalls; lastStorageTarget = target; lastStorageLevels = levels;
        lastStorageInternalFormat = internalFormat; lastStorageW = width;
        lastStorageH = height;
    }
    void storage3D(uint32_t target, int levels, uint32_t internalFormat,
                   int width, int height, int depth) override {
        ++storage3DCalls; lastStorageTarget = target; lastStorageLevels = levels;
        lastStorageInternalFormat = internalFormat; lastStorageW = width;
        lastStorageH = height; lastStorageD = depth;
    }
    void generateMipmap(uint32_t target) override {
        ++generateMipmapCalls; lastSubTarget = target;
    }
    void textureBuffer(uint32_t target, uint32_t internalFormat,
                       uint32_t bufferNativeId) override {
        ++textureBufferCalls; lastStorageTarget = target;
        lastBufferInternalFormat = internalFormat; lastBufferNativeId = bufferNativeId;
        lastBufferOffset = 0;
    }
    void textureBufferRange(uint32_t target, uint32_t internalFormat,
                             uint32_t bufferNativeId, intptr_t offset,
                             intptr_t size) override {
        ++textureBufferRangeCalls; lastStorageTarget = target;
        lastBufferInternalFormat = internalFormat; lastBufferNativeId = bufferNativeId;
        lastBufferOffset = offset; lastBufferSize = size;
    }
    void storage2DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                              int width, int height, bool fixedSampleLocations) override {
        ++storage2DMultisampleCalls; lastMTarget = target; lastMSamples = samples;
        lastMInternalFormat = internalFormat; lastMWidth = width; lastMHeight = height;
        lastMFixed = fixedSampleLocations;
    }
    void storage3DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                              int width, int height, int depth,
                              bool fixedSampleLocations) override {
        ++storage3DMultisampleCalls; lastMTarget = target; lastMSamples = samples;
        lastMInternalFormat = internalFormat; lastMWidth = width;
        lastMHeight = height; lastMDepth = depth; lastMFixed = fixedSampleLocations;
    }
    void texImage2DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                               int width, int height, bool fixedSampleLocations) override {
        ++texImage2DMultisampleCalls; lastMTarget = target; lastMSamples = samples;
        lastMInternalFormat = internalFormat; lastMWidth = width; lastMHeight = height;
        lastMFixed = fixedSampleLocations;
    }
    void texImage3DMultisample(uint32_t target, int samples, uint32_t internalFormat,
                               int width, int height, int depth,
                               bool fixedSampleLocations) override {
        ++texImage3DMultisampleCalls; lastMTarget = target; lastMSamples = samples;
        lastMInternalFormat = internalFormat; lastMWidth = width;
        lastMHeight = height; lastMDepth = depth; lastMFixed = fixedSampleLocations;
    }
    void getLevelParameteriv(uint32_t target, int level, uint32_t pname,
                             int32_t* params) override {
        ++getLevelParameterCalls; lastStorageTarget = target;
        lastLevelParamLevel = level; lastLevelParamPname = pname; (void)params;
    }
    void getLevelParameterfv(uint32_t target, int level, uint32_t pname,
                             float* params) override {
        ++getLevelParameterCalls; lastStorageTarget = target;
        lastLevelParamLevel = level; lastLevelParamPname = pname; (void)params;
    }
    void getTexImage(uint32_t target, int level, uint32_t format, uint32_t type,
                     void* pixels) override {
        ++getTexImageCalls; lastSubTarget = target; lastSubLevel = level;
        lastCopyInternalFormat = format; lastCopyBorder = static_cast<int>(type);
        (void)pixels;
    }
    uint32_t nativeId() const override { return static_cast<uint32_t>(id); }
};
class MockRenderbuffer : public BackendRenderbuffer {
public:
    int id = 0;
    int renderbufferStorageCalls = 0;
    uint32_t lastTarget = 0;
    uint32_t lastInternalFormat = 0;
    int lastWidth = 0;
    int lastHeight = 0;
    int renderbufferStorageMultisampleCalls = 0;
    int lastSamples = 0;
    void renderbufferStorage(uint32_t target, uint32_t internalFormat, int width,
                            int height) override {
        ++renderbufferStorageCalls;
        lastTarget = target;
        lastInternalFormat = internalFormat;
        lastWidth = width;
        lastHeight = height;
    }
    void renderbufferStorageMultisample(uint32_t target, int samples,
                                       uint32_t internalFormat, int width,
                                       int height) override {
        ++renderbufferStorageMultisampleCalls;
        lastTarget = target;
        lastSamples = samples;
        lastInternalFormat = internalFormat;
        lastWidth = width;
        lastHeight = height;
    }
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
    int framebufferTextureLayerCalls = 0;
    uint32_t lastLayerNativeTexture = 0;
    int lastLayerLevel = 0;
    int lastLayer = 0;
    void framebufferTextureLayer(uint32_t target, uint32_t attachment,
                                uint32_t nativeTexture, int level,
                                int layer) override {
        ++framebufferTextureLayerCalls;
        lastTarget = target;
        lastAttachment = attachment;
        lastLayerNativeTexture = nativeTexture;
        lastLayerLevel = level;
        lastLayer = layer;
    }
    int framebufferParameteriCalls = 0;
    uint32_t lastParamPname = 0;
    int lastParamValue = 0;
    void framebufferParameteri(uint32_t target, uint32_t pname,
                              int param) override {
        ++framebufferParameteriCalls;
        lastTarget = target;
        lastParamPname = pname;
        lastParamValue = param;
    }
};
class MockVertexArray : public BackendVertexArray {
public:
    int id = 0;
};
class MockSampler : public BackendSampler {
public:
    int id = 0;
    int samplerParameteriCalls = 0;
    uint32_t lastParamPname = 0;
    int lastParam = 0;
    void samplerParameteri(uint32_t pname, int param) override {
        ++samplerParameteriCalls;
        lastParamPname = pname;
        lastParam = param;
    }
    uint32_t nativeId() const override { return static_cast<uint32_t>(id); }
};
class MockTransformFeedback : public BackendTransformFeedback {
public:
    int id = 0;
    int beginCalls = 0, endCalls = 0, pauseCalls = 0, resumeCalls = 0;
    uint32_t lastBeginMode = 0;
    void begin(uint32_t mode) override { ++beginCalls; lastBeginMode = mode; }
    void end() override { ++endCalls; }
    void pause() override { ++pauseCalls; }
    void resume() override { ++resumeCalls; }
};
class MockQuery : public BackendQuery {
public:
    int id = 0;
    int beginCalls = 0, endCalls = 0;
    uint32_t lastBeginTarget = 0;
    // Test-injected result so getQueryObject* can be exercised deterministically.
    int64_t resultValue = 0;
    bool hasResult = false;
    void begin(uint32_t target) override { ++beginCalls; lastBeginTarget = target; }
    void end() override { ++endCalls; }
    void queryResult(int64_t* value, bool* available) override {
        *value = resultValue;
        *available = hasResult;
    }
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
        // A prior glBindAttribLocation takes precedence over the mock's
        // auto-assigned location (SPEC §7.3.7: bound locations are authoritative).
        auto bound = boundAttribLocations.find(name);
        if (bound != boundAttribLocations.end()) return bound->second;
        auto it = attribLocations.find(name);
        if (it != attribLocations.end()) return it->second;
        // Assign a stable, deterministic location per name (like a driver would).
        int loc = static_cast<int>(attribLocations.size());
        const_cast<MockProgram*>(this)->attribLocations[name] = loc;
        return loc;
    }
    // Records a glBindAttribLocation request (SPEC §7.3.7). Observable in tests.
    void bindAttribLocation(const std::string& name, int index) override {
        boundAttribLocations[name] = index;
    }
    std::map<std::string, int> boundAttribLocations;
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
