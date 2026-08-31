#pragma once

#include "glcompat/core/backend_resources.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

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
    int namedBufferDataCalls = 0;
    intptr_t lastNamedSize = 0;
    uint32_t lastNamedUsage = 0;
    bool lastNamedHadData = false;
    void namedBufferData(intptr_t size, uint32_t usage, const void* data) override {
        ++namedBufferDataCalls;
        lastNamedSize = size;
        lastNamedUsage = usage;
        lastNamedHadData = (data != nullptr);
    }
    int namedBufferSubDataCalls = 0;
    intptr_t lastNamedSubOffset = 0;
    intptr_t lastNamedSubSize = 0;
    bool lastNamedSubHadData = false;
    void namedBufferSubData(intptr_t offset, intptr_t size,
                            const void* data) override {
        ++namedBufferSubDataCalls;
        lastNamedSubOffset = offset;
        lastNamedSubSize = size;
        lastNamedSubHadData = (data != nullptr);
    }
    int namedBufferStorageCalls = 0;
    intptr_t lastNamedStorageSize = 0;
    uint32_t lastNamedStorageFlags = 0;
    bool lastNamedStorageHadData = false;
    void namedBufferStorage(intptr_t size, uint32_t flags,
                            const void* data) override {
        ++namedBufferStorageCalls;
        lastNamedStorageSize = size;
        lastNamedStorageFlags = flags;
        lastNamedStorageHadData = (data != nullptr);
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
    int flushMappedBufferRangeCalls = 0;
    intptr_t lastFlushOffset = 0;
    intptr_t lastFlushLength = 0;
    void flushMappedBufferRange(uint32_t target, intptr_t offset,
                                intptr_t length) override {
        ++flushMappedBufferRangeCalls;
        lastTarget = target;
        lastFlushOffset = offset;
        lastFlushLength = length;
    }
    int mapNamedBufferRangeCalls = 0;
    intptr_t lastNamedMapOffset = 0;
    intptr_t lastNamedMapLength = 0;
    uint32_t lastNamedMapAccess = 0;
    void* mapNamedBufferRange(intptr_t offset, intptr_t length,
                              uint32_t access) override {
        ++mapNamedBufferRangeCalls;
        lastNamedMapOffset = offset;
        lastNamedMapLength = length;
        lastNamedMapAccess = access;
        return nullptr; // frontend serves the CPU mirror
    }
    int unmapNamedBufferCalls = 0;
    void unmapNamedBuffer() override { ++unmapNamedBufferCalls; }
    int flushMappedNamedBufferRangeCalls = 0;
    intptr_t lastNamedFlushOffset = 0;
    intptr_t lastNamedFlushLength = 0;
    void flushMappedNamedBufferRange(intptr_t offset, intptr_t length) override {
        ++flushMappedNamedBufferRangeCalls;
        lastNamedFlushOffset = offset;
        lastNamedFlushLength = length;
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
    int copyTexSubImage1DCalls = 0, copyTexSubImage2DCalls = 0,
        copyTexSubImage3DCalls = 0;
    uint32_t lastCopyInternalFormat = 0;
    int lastCopyX = 0, lastCopyY = 0;
    int lastCopyWidth = 0, lastCopyHeight = 0, lastCopyBorder = 0;
    int lastCopySubXoffset = 0, lastCopySubYoffset = 0, lastCopySubZoffset = 0;
    int lastCopySubX = 0, lastCopySubY = 0, lastCopySubWidth = 0,
        lastCopySubHeight = 0;
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
    void copyTexSubImage1D(uint32_t target, int level, int xoffset, int x, int y,
                           int width) override {
        ++copyTexSubImage1DCalls;
        lastSubTarget = target; lastSubLevel = level;
        lastCopySubXoffset = xoffset; lastCopySubX = x; lastCopySubY = y;
        lastCopySubWidth = width;
    }
    void copyTexSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                           int x, int y, int width, int height) override {
        ++copyTexSubImage2DCalls;
        lastSubTarget = target; lastSubLevel = level;
        lastCopySubXoffset = xoffset; lastCopySubYoffset = yoffset;
        lastCopySubX = x; lastCopySubY = y;
        lastCopySubWidth = width; lastCopySubHeight = height;
    }
    void copyTexSubImage3D(uint32_t target, int level, int xoffset, int yoffset,
                           int zoffset, int x, int y, int width,
                           int height) override {
        ++copyTexSubImage3DCalls;
        lastSubTarget = target; lastSubLevel = level;
        lastCopySubXoffset = xoffset; lastCopySubYoffset = yoffset;
        lastCopySubZoffset = zoffset; lastCopySubX = x; lastCopySubY = y;
        lastCopySubWidth = width; lastCopySubHeight = height;
    }
    int compressedTexImage1DCalls = 0, compressedTexImage2DCalls = 0,
        compressedTexImage3DCalls = 0;
    int compressedTexSubImage1DCalls = 0, compressedTexSubImage2DCalls = 0,
        compressedTexSubImage3DCalls = 0;
    uint32_t lastCompressedTarget = 0, lastCompressedInternalFormat = 0,
             lastCompressedFormat = 0;
    int lastCompressedLevel = 0;
    int lastCompressedXoffset = 0, lastCompressedYoffset = 0,
        lastCompressedZoffset = 0;
    int lastCompressedWidth = 0, lastCompressedHeight = 0,
        lastCompressedDepth = 0, lastCompressedBorder = 0,
        lastCompressedImageSize = 0;
    void compressedTexImage1D(uint32_t target, int level, uint32_t internalFormat,
                              int width, int border, int imageSize,
                              const void* data) override {
        ++compressedTexImage1DCalls;
        lastCompressedTarget = target; lastCompressedLevel = level;
        lastCompressedInternalFormat = internalFormat; lastCompressedWidth = width;
        lastCompressedBorder = border; lastCompressedImageSize = imageSize;
        (void)data;
    }
    void compressedTexImage2D(uint32_t target, int level, uint32_t internalFormat,
                              int width, int height, int border, int imageSize,
                              const void* data) override {
        ++compressedTexImage2DCalls;
        lastCompressedTarget = target; lastCompressedLevel = level;
        lastCompressedInternalFormat = internalFormat; lastCompressedWidth = width;
        lastCompressedHeight = height; lastCompressedBorder = border;
        lastCompressedImageSize = imageSize;
        (void)data;
    }
    void compressedTexImage3D(uint32_t target, int level, uint32_t internalFormat,
                              int width, int height, int depth, int border,
                              int imageSize, const void* data) override {
        ++compressedTexImage3DCalls;
        lastCompressedTarget = target; lastCompressedLevel = level;
        lastCompressedInternalFormat = internalFormat; lastCompressedWidth = width;
        lastCompressedHeight = height; lastCompressedDepth = depth;
        lastCompressedBorder = border; lastCompressedImageSize = imageSize;
        (void)data;
    }
    void compressedTexSubImage1D(uint32_t target, int level, int xoffset, int width,
                                 uint32_t format, int imageSize,
                                 const void* data) override {
        ++compressedTexSubImage1DCalls;
        lastCompressedTarget = target; lastCompressedLevel = level;
        lastCompressedXoffset = xoffset; lastCompressedWidth = width;
        lastCompressedFormat = format; lastCompressedImageSize = imageSize;
        (void)data;
    }
    void compressedTexSubImage2D(uint32_t target, int level, int xoffset,
                                 int yoffset, int width, int height,
                                 uint32_t format, int imageSize,
                                 const void* data) override {
        ++compressedTexSubImage2DCalls;
        lastCompressedTarget = target; lastCompressedLevel = level;
        lastCompressedXoffset = xoffset; lastCompressedYoffset = yoffset;
        lastCompressedWidth = width; lastCompressedHeight = height;
        lastCompressedFormat = format; lastCompressedImageSize = imageSize;
        (void)data;
    }
    void compressedTexSubImage3D(uint32_t target, int level, int xoffset,
                                 int yoffset, int zoffset, int width, int height,
                                 int depth, uint32_t format, int imageSize,
                                 const void* data) override {
        ++compressedTexSubImage3DCalls;
        lastCompressedTarget = target; lastCompressedLevel = level;
        lastCompressedXoffset = xoffset; lastCompressedYoffset = yoffset;
        lastCompressedZoffset = zoffset; lastCompressedWidth = width;
        lastCompressedHeight = height; lastCompressedDepth = depth;
        lastCompressedFormat = format; lastCompressedImageSize = imageSize;
        (void)data;
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
    int viewCalls = 0;
    uint32_t lastViewTarget = 0, lastViewOrigNativeId = 0, lastViewInternalFormat = 0;
    uint32_t lastViewMinLevel = 0, lastViewNumLevels = 0, lastViewMinLayer = 0,
             lastViewNumLayers = 0;
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
    void view(uint32_t target, uint32_t origTextureNativeId, uint32_t internalFormat,
              uint32_t minLevel, uint32_t numLevels, uint32_t minLayer,
              uint32_t numLayers) override {
        ++viewCalls; lastViewTarget = target; lastViewOrigNativeId = origTextureNativeId;
        lastViewInternalFormat = internalFormat; lastViewMinLevel = minLevel;
        lastViewNumLevels = numLevels; lastViewMinLayer = minLayer;
        lastViewNumLayers = numLayers;
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
    int getCompressedTexImageCalls = 0;
    void getCompressedTexImage(uint32_t target, int level, void* pixels) override {
        ++getCompressedTexImageCalls; lastSubTarget = target; lastSubLevel = level;
        (void)pixels;
    }
    int getTexImageRobustCalls = 0;
    void getTexImage(uint32_t target, int level, uint32_t format, uint32_t type,
                     int bufSize, void* pixels) override {
        ++getTexImageRobustCalls; lastSubTarget = target; lastSubLevel = level;
        lastCopyInternalFormat = format; lastCopyBorder = static_cast<int>(type);
        (void)bufSize; (void)pixels;
    }
    int getCompressedTexImageRobustCalls = 0;
    void getCompressedTexImage(uint32_t target, int level, int bufSize,
                               void* pixels) override {
        ++getCompressedTexImageRobustCalls; lastSubTarget = target;
        lastSubLevel = level; (void)bufSize; (void)pixels;
    }
    int getTextureSubImageCalls = 0;
    void getTextureSubImage(uint32_t target, int level, int xoffset, int yoffset,
                            int zoffset, int width, int height, int depth,
                            uint32_t format, uint32_t type, int bufSize,
                            void* pixels) override {
        ++getTextureSubImageCalls; lastSubTarget = target; lastSubLevel = level;
        (void)xoffset; (void)yoffset; (void)zoffset; (void)width; (void)height;
        (void)depth; (void)format; (void)type; (void)bufSize; (void)pixels;
    }
    int getCompressedTextureSubImageCalls = 0;
    void getCompressedTextureSubImage(uint32_t target, int level, int xoffset,
                                      int yoffset, int zoffset, int width, int height,
                                      int depth, int bufSize, void* pixels) override {
        ++getCompressedTextureSubImageCalls; lastSubTarget = target;
        lastSubLevel = level;
        (void)xoffset; (void)yoffset; (void)zoffset; (void)width; (void)height;
        (void)depth; (void)bufSize; (void)pixels;
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
    int samplerParameterfCalls = 0;
    float lastParamf = 0.0f;
    void samplerParameterf(uint32_t pname, float param) override {
        ++samplerParameterfCalls;
        lastParamPname = pname;
        lastParamf = param;
    }
    int samplerParameterfvCalls = 0;
    std::vector<float> lastParamfv;
    void samplerParameterfv(uint32_t pname, const float* params, int count) override {
        ++samplerParameterfvCalls;
        lastParamPname = pname;
        if (params && count > 0) lastParamfv.assign(params, params + count);
    }
    int samplerParameterivCalls = 0;
    std::vector<int32_t> lastParamiv;
    void samplerParameteriv(uint32_t pname, const int32_t* params, int count) override {
        ++samplerParameterivCalls;
        lastParamPname = pname;
        if (params && count > 0) lastParamiv.assign(params, params + count);
    }
    int samplerParameterIivCalls = 0;
    std::vector<int32_t> lastParamIiv;
    void samplerParameterIiv(uint32_t pname, const int32_t* params) override {
        ++samplerParameterIivCalls;
        lastParamPname = pname;
        if (params) lastParamIiv.assign(params, params + 1);
    }
    int samplerParameterIuivCalls = 0;
    std::vector<uint32_t> lastParamIuiv;
    void samplerParameterIuiv(uint32_t pname, const uint32_t* params) override {
        ++samplerParameterIuivCalls;
        lastParamPname = pname;
        if (params) lastParamIuiv.assign(params, params + 1);
    }
    uint32_t nativeId() const override { return static_cast<uint32_t>(id); }
};
class MockTransformFeedback : public BackendTransformFeedback {
public:
    int id = 0;
    int beginCalls = 0, endCalls = 0, pauseCalls = 0, resumeCalls = 0;
    uint32_t lastBeginMode = 0;
    // Test-injected captured vertex count so drawTransformFeedback* can be
    // exercised deterministically (SPEC §13.3.3).
    int64_t capturedVertexCount = 0;
    void begin(uint32_t mode) override { ++beginCalls; lastBeginMode = mode; }
    void end() override { ++endCalls; }
    void pause() override { ++pauseCalls; }
    void resume() override { ++resumeCalls; }
    int64_t getCapturedVertexCount(uint32_t stream = 0) const override {
        (void)stream;
        return capturedVertexCount;
    }
};
class MockQuery : public BackendQuery {
public:
    int id = 0;
    int beginCalls = 0, endCalls = 0;
    int queryCounterCalls = 0;
    uint32_t lastBeginTarget = 0;
    uint32_t lastCounterTarget = 0;
    // Test-injected result so getQueryObject* can be exercised deterministically.
    int64_t resultValue = 0;
    bool hasResult = false;
    void begin(uint32_t target) override { ++beginCalls; lastBeginTarget = target; }
    void end() override { ++endCalls; }
    void queryCounter(uint32_t target) override { ++queryCounterCalls; lastCounterTarget = target; }
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
    // glShaderBinary recording (observable in tests).
    int loadBinaryCalls = 0;
    uint32_t lastBinaryFormat = 0;
    int lastBinaryLength = 0;
    void loadBinary(uint32_t binaryFormat, const void* binary, int32_t length) override {
        ++loadBinaryCalls;
        lastBinaryFormat = binaryFormat;
        lastBinaryLength = static_cast<int>(length);
        (void)binary;
    }
    // glSpecializeShader recording (observable in tests).
    int specializeCalls = 0;
    std::string lastEntryPoint;
    uint32_t lastNumConstants = 0;
    std::vector<uint32_t> lastConstantIndex;
    std::vector<uint32_t> lastConstantValue;
    bool specialize(const std::string& entryPoint, uint32_t numConstants,
                    const uint32_t* constantIndex, const uint32_t* constantValue,
                    std::string& log) override {
        ++specializeCalls;
        lastEntryPoint = entryPoint;
        lastNumConstants = numConstants;
        lastConstantIndex.assign(constantIndex, constantIndex + numConstants);
        lastConstantValue.assign(constantValue, constantValue + numConstants);
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
    void detach(BackendShader&) override {
        // The frontend drives link() from its own attachedShaders list, so removing
        // a shader there is sufficient; the mock program needs no native detach.
    }
    bool link(std::string& log) override {
        if (attached.empty()) {
            log = "no shaders attached";
            return false;
        }
        log.clear();
        return true;
    }
    int validateCalls = 0;
    void validate(std::string& log) override {
        ++validateCalls;
        log = "validated";
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
    int getFragDataLocation(const std::string& name) const override {
        auto it = fragDataLocations.find(name);
        if (it != fragDataLocations.end()) return it->second;
        return -1; // not an active fragment output
    }
    int getFragDataIndex(const std::string& name) const override {
        auto it = fragDataIndices.find(name);
        if (it != fragDataIndices.end()) return it->second;
        return -1; // not an active fragment output
    }
    struct MockTfVarying {
        std::string name;
        int size = 1;
        uint32_t type = 0x1406; // GL_FLOAT
    };
    bool getTransformFeedbackVarying(uint32_t index, int bufSize, int* length,
                                     int* size, uint32_t* type, char* name) const override {
        if (index >= tfVaryings.size()) return false;
        const MockTfVarying& v = tfVaryings[index];
        if (size) *size = v.size;
        if (type) *type = v.type;
        int n = static_cast<int>(v.name.size());
        if (length) *length = n;
        if (name && bufSize > 0) {
            int copy = std::min(n, bufSize - 1);
            std::memcpy(name, v.name.data(), static_cast<size_t>(copy));
            name[copy] = '\0';
        }
        return true;
    }
    std::vector<MockTfVarying> tfVaryings;
    std::map<std::string, int> fragDataLocations;
    std::map<std::string, int> fragDataIndices;
    // Records a glBindFragDataLocation / glBindFragDataLocationIndexed request
    // (SPEC §7.3.7 / §15.1.2). Observable in tests; consumed by getFragData-
    // Location / getFragDataIndex after link.
    void bindFragDataLocation(const std::string& name, int colorNumber,
                              int index) override {
        fragDataLocations[name] = colorNumber;
        fragDataIndices[name] = index;
    }
    // Records a glBindAttribLocation request (SPEC §7.3.7). Observable in tests.
    void bindAttribLocation(const std::string& name, int index) override {
        boundAttribLocations[name] = index;
    }
    std::map<std::string, int> boundAttribLocations;
    // Uniform-block binding recording (SPEC §7.6.2). `activeUniformBlocks` lets
    // the frontend's blockIndex validation pass (else INVALID_VALUE); the mock
    // records the block index -> binding-point association for test observability
    // and reports it back through glGetActiveUniformBlockiv(UNIFORM_BLOCK_BINDING).
    uint32_t activeUniformBlocks = 0;
    std::map<uint32_t, uint32_t> blockBindings;
    int activeUniformBlockCount() const override {
        return static_cast<int>(activeUniformBlocks);
    }
    void uniformBlockBinding(uint32_t blockIndex, uint32_t blockBinding) override {
        blockBindings[blockIndex] = blockBinding;
    }
    // Shader-storage block -> binding-point association (SPEC §7.6.2
    // glShaderStorageBlockBinding). Mirrors the uniform-block recording above.
    uint32_t activeShaderStorageBlocks = 0;
    std::map<uint32_t, uint32_t> shaderStorageBlockBindings;
    int activeShaderStorageBlockCount() const override {
        return static_cast<int>(activeShaderStorageBlocks);
    }
    void shaderStorageBlockBinding(uint32_t blockIndex, uint32_t blockBinding) override {
        shaderStorageBlockBindings[blockIndex] = blockBinding;
    }
    void transformFeedbackVaryings(const std::vector<std::string>& varyings,
                                   uint32_t bufferMode) override {
        tfRequestedVaryings = varyings;
        tfRequestedBufferMode = bufferMode;
    }
    std::vector<std::string> tfRequestedVaryings;
    uint32_t tfRequestedBufferMode = 0;
    // Active atomic-counter buffer reflection (SPEC §7.7) for test observability.
    struct AtomicCounterBufferInfo {
        int32_t binding = 0;
        int32_t dataSize = 0;
        std::vector<int32_t> indices;
        int32_t referencedByVertex = 0;
        int32_t referencedByTessControl = 0;
        int32_t referencedByTessEval = 0;
        int32_t referencedByGeometry = 0;
        int32_t referencedByFragment = 0;
        int32_t referencedByCompute = 0;
    };
    std::vector<AtomicCounterBufferInfo> atomicCounterBuffers;
    uint32_t atomicCounterBufferCount() const {
        return static_cast<uint32_t>(atomicCounterBuffers.size());
    }
    void getProgramResourceiv(uint32_t programInterface, uint32_t index,
                              int32_t propCount, const uint32_t* props,
                              int32_t bufSize, int32_t* length,
                              int32_t* params) const override {
        if (programInterface == GL_UNIFORM_BLOCK && propCount >= 1 &&
            props[0] == GL_BUFFER_BINDING && params && bufSize >= 1) {
            auto it = blockBindings.find(index);
            params[0] = (it != blockBindings.end())
                            ? static_cast<int32_t>(it->second)
                            : 0;
            if (length) *length = 1;
            return;
        }
        if (programInterface == GL_ATOMIC_COUNTER_BUFFER) {
            if (index >= atomicCounterBuffers.size() || params == nullptr) return;
            const AtomicCounterBufferInfo& acb = atomicCounterBuffers[index];
            for (int32_t p = 0; p < propCount; ++p) {
                uint32_t prop = props[p];
                int32_t val = 0;
                if (prop == GL_BUFFER_BINDING) val = acb.binding;
                else if (prop == GL_BUFFER_DATA_SIZE) val = acb.dataSize;
                else if (prop == GL_NUM_ACTIVE_VARIABLES)
                    val = static_cast<int32_t>(acb.indices.size());
                else if (prop == GL_ACTIVE_VARIABLES) {
                    int32_t n = static_cast<int32_t>(acb.indices.size());
                    int32_t out = (bufSize < n) ? bufSize : n;
                    if (length) *length = n;
                    for (int32_t i = 0; i < out; ++i) params[i] = acb.indices[i];
                    continue;
                } else if (prop == GL_REFERENCED_BY_VERTEX_SHADER)
                    val = acb.referencedByVertex;
                else if (prop == GL_REFERENCED_BY_TESS_CONTROL_SHADER)
                    val = acb.referencedByTessControl;
                else if (prop == GL_REFERENCED_BY_TESS_EVALUATION_SHADER)
                    val = acb.referencedByTessEval;
                else if (prop == GL_REFERENCED_BY_GEOMETRY_SHADER)
                    val = acb.referencedByGeometry;
                else if (prop == GL_REFERENCED_BY_FRAGMENT_SHADER)
                    val = acb.referencedByFragment;
                else if (prop == GL_REFERENCED_BY_COMPUTE_SHADER)
                    val = acb.referencedByCompute;
                if (bufSize > p) params[p] = val;
            }
            return;
        }
    }
    uint32_t nativeId() const override { return static_cast<uint32_t>(id); }


    // glGetProgramInterfaceiv recording (SPEC §7.3.1). ACTIVE_RESOURCES is taken
    // from the configurable count below; MAX_* sizing pnames fall back to 0 unless
    // explicitly set in interfaceCounts.
    uint32_t interfaceActiveResources = 0;
    int programInterfaceCalls = 0;
    uint32_t lastInterface = 0;
    uint32_t lastInterfacePname = 0;
    int32_t lastInterfaceResult = 0;
    std::map<uint32_t, int32_t> interfaceCounts;
    uint32_t programResourceCount(uint32_t programInterface) const override {
        if (programInterface == GL_UNIFORM_BLOCK) return activeUniformBlocks;
        if (programInterface == GL_ATOMIC_COUNTER_BUFFER)
            return atomicCounterBufferCount();
        return interfaceActiveResources;
    }
    void getProgramInterfaceiv(uint32_t programInterface, uint32_t pname,
                               int32_t* params) const override {
        ++const_cast<MockProgram*>(this)->programInterfaceCalls;
        const_cast<MockProgram*>(this)->lastInterface = programInterface;
        const_cast<MockProgram*>(this)->lastInterfacePname = pname;
        int32_t val = 0;
        auto it = interfaceCounts.find(pname);
        if (it != interfaceCounts.end()) {
            val = it->second;
        } else if (pname == GL_ACTIVE_RESOURCES) {
            val = static_cast<int32_t>(programResourceCount(programInterface));
        }
        const_cast<MockProgram*>(this)->lastInterfaceResult = val;
        if (params) *params = val;
    }


    // glProgramBinary recording (observable in tests).
    int loadBinaryCalls = 0;
    uint32_t lastBinaryFormat = 0;
    int lastBinaryLength = 0;
    void loadBinary(uint32_t binaryFormat, const void* binary, int32_t length) override {
        ++loadBinaryCalls;
        lastBinaryFormat = binaryFormat;
        lastBinaryLength = static_cast<int>(length);
        (void)binary;
    }

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
    int uniform2fvCalls = 0, uniform3fvCalls = 0, uniform4fvCalls = 0;
    int uniform2ivCalls = 0, uniform3ivCalls = 0, uniform4ivCalls = 0;
    int uniformMatrix2fvCalls = 0, uniformMatrix3fvCalls = 0;
    int uniform1dCalls = 0, uniform2dCalls = 0, uniform3dCalls = 0, uniform4dCalls = 0;
    int uniform1dvCalls = 0, uniform2dvCalls = 0, uniform3dvCalls = 0, uniform4dvCalls = 0;
    int uniformMatrix2dvCalls = 0, uniformMatrix3dvCalls = 0, uniformMatrix4dvCalls = 0;
    // Non-square matrix uniform call counters (SPEC §7.6).
    int uniformMatrix2x3fvCalls = 0, uniformMatrix2x3dvCalls = 0;
    int uniformMatrix2x4fvCalls = 0, uniformMatrix2x4dvCalls = 0;
    int uniformMatrix3x2fvCalls = 0, uniformMatrix3x2dvCalls = 0;
    int uniformMatrix3x4fvCalls = 0, uniformMatrix3x4dvCalls = 0;
    int uniformMatrix4x2fvCalls = 0, uniformMatrix4x2dvCalls = 0;
    int uniformMatrix4x3fvCalls = 0, uniformMatrix4x3dvCalls = 0;
    int uniform1uiCalls = 0, uniform2uiCalls = 0, uniform3uiCalls = 0, uniform4uiCalls = 0;
    int uniform1uivCalls = 0, uniform2uivCalls = 0, uniform3uivCalls = 0, uniform4uivCalls = 0;
    mutable int getUniformfvCalls = 0, getUniformivCalls = 0, getUniformuivCalls = 0,
        getUniformdvCalls = 0;
    int lastUniformLoc = -1;
    float lastF0 = 0, lastF1 = 0, lastF2 = 0, lastF3 = 0;
    int lastI0 = 0, lastI1 = 0, lastI2 = 0, lastI3 = 0;
    double lastD0 = 0, lastD1 = 0, lastD2 = 0, lastD3 = 0;
    uint32_t lastU0 = 0, lastU1 = 0, lastU2 = 0, lastU3 = 0;
    int lastUniformCount = 0;
    bool lastTranspose = false;
    // Mirrors of the most recently written uniform values, keyed by location, so
    // glGetUniform* can round-trip through the mock.
    std::unordered_map<int, std::vector<float>> uniformFloatStore;
    std::unordered_map<int, std::vector<int32_t>> uniformIntStore;
    std::unordered_map<int, std::vector<uint32_t>> uniformUintStore;
    std::unordered_map<int, std::vector<double>> uniformDoubleStore;
    void uniform1f(int loc, float v0) override {
        ++uniform1fCalls; lastUniformLoc = loc; lastF0 = v0;
        uniformFloatStore[loc] = {v0};
    }
    void uniform2f(int loc, float v0, float v1) override {
        ++uniform2fCalls; lastUniformLoc = loc; lastF0 = v0; lastF1 = v1;
        uniformFloatStore[loc] = {v0, v1};
    }
    void uniform3f(int loc, float v0, float v1, float v2) override {
        ++uniform3fCalls; lastUniformLoc = loc; lastF0 = v0; lastF1 = v1; lastF2 = v2;
        uniformFloatStore[loc] = {v0, v1, v2};
    }
    void uniform4f(int loc, float v0, float v1, float v2, float v3) override {
        ++uniform4fCalls; lastUniformLoc = loc; lastF0 = v0; lastF1 = v1;
        lastF2 = v2; lastF3 = v3;
        uniformFloatStore[loc] = {v0, v1, v2, v3};
    }
    void uniform1i(int loc, int v0) override {
        ++uniform1iCalls; lastUniformLoc = loc; lastI0 = v0;
        uniformIntStore[loc] = {v0};
    }
    void uniform2i(int loc, int v0, int v1) override {
        ++uniform2iCalls; lastUniformLoc = loc; lastI0 = v0; lastI1 = v1;
        uniformIntStore[loc] = {v0, v1};
    }
    void uniform3i(int loc, int v0, int v1, int v2) override {
        ++uniform3iCalls; lastUniformLoc = loc; lastI0 = v0; lastI1 = v1; lastI2 = v2;
        uniformIntStore[loc] = {v0, v1, v2};
    }
    void uniform4i(int loc, int v0, int v1, int v2, int v3) override {
        ++uniform4iCalls; lastUniformLoc = loc; lastI0 = v0; lastI1 = v1;
        lastI2 = v2; lastI3 = v3;
        uniformIntStore[loc] = {v0, v1, v2, v3};
    }
    void uniform1fv(int loc, const float* v, int count) override {
        ++uniform1fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) {
            lastF0 = v[0];
            uniformFloatStore[loc].assign(v, v + count);
        }
    }
    void uniform1iv(int loc, const int* v, int count) override {
        ++uniform1ivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) {
            lastI0 = v[0];
            uniformIntStore[loc].assign(v, v + count);
        }
    }
    void uniformMatrix4fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix4fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) {
            uniformFloatStore[loc].assign(m, m + 16 * count);
        }
    }
    void uniform2fv(int loc, const float* v, int count) override {
        ++uniform2fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastF0 = v[0]; uniformFloatStore[loc].assign(v, v + 2 * count); }
    }
    void uniform3fv(int loc, const float* v, int count) override {
        ++uniform3fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastF0 = v[0]; uniformFloatStore[loc].assign(v, v + 3 * count); }
    }
    void uniform4fv(int loc, const float* v, int count) override {
        ++uniform4fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastF0 = v[0]; uniformFloatStore[loc].assign(v, v + 4 * count); }
    }
    void uniform2iv(int loc, const int* v, int count) override {
        ++uniform2ivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastI0 = v[0]; uniformIntStore[loc].assign(v, v + 2 * count); }
    }
    void uniform3iv(int loc, const int* v, int count) override {
        ++uniform3ivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastI0 = v[0]; uniformIntStore[loc].assign(v, v + 3 * count); }
    }
    void uniform4iv(int loc, const int* v, int count) override {
        ++uniform4ivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastI0 = v[0]; uniformIntStore[loc].assign(v, v + 4 * count); }
    }
    void uniformMatrix2fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix2fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformFloatStore[loc].assign(m, m + 4 * count);
    }
    void uniformMatrix3fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix3fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformFloatStore[loc].assign(m, m + 9 * count);
    }
    void uniform1d(int loc, double v0) override {
        ++uniform1dCalls; lastUniformLoc = loc; lastD0 = v0;
        uniformDoubleStore[loc] = {v0};
    }
    void uniform2d(int loc, double v0, double v1) override {
        ++uniform2dCalls; lastUniformLoc = loc; lastD0 = v0; lastD1 = v1;
        uniformDoubleStore[loc] = {v0, v1};
    }
    void uniform3d(int loc, double v0, double v1, double v2) override {
        ++uniform3dCalls; lastUniformLoc = loc; lastD0 = v0; lastD1 = v1; lastD2 = v2;
        uniformDoubleStore[loc] = {v0, v1, v2};
    }
    void uniform4d(int loc, double v0, double v1, double v2, double v3) override {
        ++uniform4dCalls; lastUniformLoc = loc; lastD0 = v0; lastD1 = v1;
        lastD2 = v2; lastD3 = v3;
        uniformDoubleStore[loc] = {v0, v1, v2, v3};
    }
    void uniform1dv(int loc, const double* v, int count) override {
        ++uniform1dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastD0 = v[0]; uniformDoubleStore[loc].assign(v, v + count); }
    }
    void uniform2dv(int loc, const double* v, int count) override {
        ++uniform2dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastD0 = v[0]; uniformDoubleStore[loc].assign(v, v + 2 * count); }
    }
    void uniform3dv(int loc, const double* v, int count) override {
        ++uniform3dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastD0 = v[0]; uniformDoubleStore[loc].assign(v, v + 3 * count); }
    }
    void uniform4dv(int loc, const double* v, int count) override {
        ++uniform4dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastD0 = v[0]; uniformDoubleStore[loc].assign(v, v + 4 * count); }
    }
    void uniformMatrix2dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix2dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 4 * count);
    }
    void uniformMatrix3dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix3dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 9 * count);
    }
    void uniformMatrix4dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix4dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 16 * count);
    }
    // Non-square matrix uniform setters (SPEC §7.6). A NxM matrix holds N*M
    // components, so the mirror store keeps count * N * M values for readback.
    void uniformMatrix2x3fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix2x3fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformFloatStore[loc].assign(m, m + 6 * count);
    }
    void uniformMatrix2x3dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix2x3dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 6 * count);
    }
    void uniformMatrix2x4fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix2x4fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformFloatStore[loc].assign(m, m + 8 * count);
    }
    void uniformMatrix2x4dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix2x4dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 8 * count);
    }
    void uniformMatrix3x2fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix3x2fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformFloatStore[loc].assign(m, m + 6 * count);
    }
    void uniformMatrix3x2dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix3x2dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 6 * count);
    }
    void uniformMatrix3x4fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix3x4fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformFloatStore[loc].assign(m, m + 12 * count);
    }
    void uniformMatrix3x4dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix3x4dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 12 * count);
    }
    void uniformMatrix4x2fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix4x2fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformFloatStore[loc].assign(m, m + 8 * count);
    }
    void uniformMatrix4x2dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix4x2dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 8 * count);
    }
    void uniformMatrix4x3fv(int loc, const float* m, int count, bool transpose) override {
        ++uniformMatrix4x3fvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformFloatStore[loc].assign(m, m + 12 * count);
    }
    void uniformMatrix4x3dv(int loc, const double* m, int count, bool transpose) override {
        ++uniformMatrix4x3dvCalls; lastUniformLoc = loc; lastUniformCount = count;
        lastTranspose = transpose;
        if (m && count > 0) uniformDoubleStore[loc].assign(m, m + 12 * count);
    }
    void uniform1ui(int loc, uint32_t v0) override {
        ++uniform1uiCalls; lastUniformLoc = loc; lastU0 = v0;
        uniformUintStore[loc] = {v0};
    }
    void uniform2ui(int loc, uint32_t v0, uint32_t v1) override {
        ++uniform2uiCalls; lastUniformLoc = loc; lastU0 = v0; lastU1 = v1;
        uniformUintStore[loc] = {v0, v1};
    }
    void uniform3ui(int loc, uint32_t v0, uint32_t v1, uint32_t v2) override {
        ++uniform3uiCalls; lastUniformLoc = loc; lastU0 = v0; lastU1 = v1; lastU2 = v2;
        uniformUintStore[loc] = {v0, v1, v2};
    }
    void uniform4ui(int loc, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3) override {
        ++uniform4uiCalls; lastUniformLoc = loc; lastU0 = v0; lastU1 = v1;
        lastU2 = v2; lastU3 = v3;
        uniformUintStore[loc] = {v0, v1, v2, v3};
    }
    void uniform1uiv(int loc, const uint32_t* v, int count) override {
        ++uniform1uivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastU0 = v[0]; uniformUintStore[loc].assign(v, v + count); }
    }
    void uniform2uiv(int loc, const uint32_t* v, int count) override {
        ++uniform2uivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastU0 = v[0]; uniformUintStore[loc].assign(v, v + 2 * count); }
    }
    void uniform3uiv(int loc, const uint32_t* v, int count) override {
        ++uniform3uivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastU0 = v[0]; uniformUintStore[loc].assign(v, v + 3 * count); }
    }
    void uniform4uiv(int loc, const uint32_t* v, int count) override {
        ++uniform4uivCalls; lastUniformLoc = loc; lastUniformCount = count;
        if (v && count > 0) { lastU0 = v[0]; uniformUintStore[loc].assign(v, v + 4 * count); }
    }
    void getUniformfv(int32_t location, float* params) const override {
        ++getUniformfvCalls;
        auto it = uniformFloatStore.find(location);
        if (it != uniformFloatStore.end() && params) {
            for (size_t i = 0; i < it->second.size(); ++i) params[i] = it->second[i];
        }
    }
    void getUniformiv(int32_t location, int32_t* params) const override {
        ++getUniformivCalls;
        auto it = uniformIntStore.find(location);
        if (it != uniformIntStore.end() && params) {
            for (size_t i = 0; i < it->second.size(); ++i) params[i] = it->second[i];
        }
    }
    void getUniformuiv(int32_t location, uint32_t* params) const override {
        ++getUniformuivCalls;
        auto it = uniformUintStore.find(location);
        if (it != uniformUintStore.end() && params) {
            for (size_t i = 0; i < it->second.size(); ++i) params[i] = it->second[i];
        }
    }
    void getUniformdv(int32_t location, double* params) const override {
        ++getUniformdvCalls;
        auto it = uniformDoubleStore.find(location);
        if (it != uniformDoubleStore.end()) {
            if (params)
                for (size_t i = 0; i < it->second.size(); ++i) params[i] = it->second[i];
            return;
        }
        auto fit = uniformFloatStore.find(location);
        if (fit != uniformFloatStore.end() && params) {
            for (size_t i = 0; i < fit->second.size(); ++i) params[i] = fit->second[i];
        }
    }

    std::unordered_map<std::string, int> attribLocations;
    std::unordered_map<std::string, int> uniformLocations;
};

} // namespace glcompat
