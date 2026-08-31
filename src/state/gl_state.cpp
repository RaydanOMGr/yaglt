#include "glcompat/state/gl_state.hpp"

namespace glcompat {

GLStateTracker::GLStateTracker() {
    texUnits_.resize(kMaxTextureUnits);
    texUnitsApplied_.resize(kMaxTextureUnits);
    samplerBound_.assign(kMaxTextureUnits, 0);
    samplerBoundApplied_.assign(kMaxTextureUnits, 0);
    imageUnit_.assign(kMaxImageUnits, ImageUnitBinding{});
    imageUnitApplied_.assign(kMaxImageUnits, ImageUnitBinding{});
    blendBuf_.assign(kMaxDrawBuffers, BlendState{});
    blendBufApplied_.assign(kMaxDrawBuffers, BlendState{});
    colorMask_.assign(kMaxDrawBuffers, ColorMaskState{});
    colorMaskApplied_.assign(kMaxDrawBuffers, ColorMaskState{});
    // SPEC §17.3.7: dithering is enabled by default.
    capsCurrent_[0x0BD0 /* GL_DITHER */] = true;
    capsApplied_[0x0BD0 /* GL_DITHER */] = true;
}

bool GLStateTracker::setCapability(GLenum cap, bool enabled) {
    auto it = capsCurrent_.find(cap);
    if (it != capsCurrent_.end() && it->second == enabled) {
        return false;
    }
    capsCurrent_[cap] = enabled;
    capsDirty_ = true;
    return true;
}

bool GLStateTracker::isCapabilityEnabled(GLenum cap) const {
    auto it = capsCurrent_.find(cap);
    return it != capsCurrent_.end() && it->second;
}

bool GLStateTracker::setHint(GLenum target, GLenum mode) {
    auto it = hints_.find(target);
    if (it != hints_.end() && it->second == mode) {
        return false;
    }
    hints_[target] = mode;
    hintsDirty_ = true;
    return true;
}

GLenum GLStateTracker::getHint(GLenum target) const {
    auto it = hints_.find(target);
    return it == hints_.end() ? GL_DONT_CARE : it->second;
}

bool GLStateTracker::setIndexedCapability(GLenum cap, uint32_t index, bool enabled) {
    auto& m = indexedCapsCurrent_[cap];
    auto it = m.find(index);
    if (it != m.end() && it->second == enabled) {
        return false;
    }
    m[index] = enabled;
    indexedCapsDirty_ = true;
    return true;
}

bool GLStateTracker::isIndexedCapabilityEnabled(GLenum cap, uint32_t index,
                                                bool* enabled) const {
    auto it = indexedCapsCurrent_.find(cap);
    if (it == indexedCapsCurrent_.end()) {
        *enabled = false;
        return true;
    }
    auto jt = it->second.find(index);
    *enabled = (jt != it->second.end() && jt->second);
    return true;
}

bool GLStateTracker::useProgram(GLObjectName prog) {
    if (activeProgram_ == prog) return false;
    activeProgram_ = prog;
    programDirty_ = true;
    return true;
}

bool GLStateTracker::bindProgramPipeline(GLObjectName pipeline) {
    if (boundProgramPipeline_ == pipeline) return false;
    boundProgramPipeline_ = pipeline;
    programPipelineDirty_ = true;
    return true;
}

bool GLStateTracker::setBlendFunc(GLenum sfactor, GLenum dfactor) {
    BlendState& b = blendBuf_[0];
    if (b.srcRGB == sfactor && b.dstRGB == dfactor &&
        b.srcAlpha == sfactor && b.dstAlpha == dfactor)
        return false;
    b.srcRGB = b.srcAlpha = sfactor;
    b.dstRGB = b.dstAlpha = dfactor;
    return true;
}

bool GLStateTracker::setBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
                                          GLenum srcAlpha, GLenum dstAlpha) {
    BlendState& b = blendBuf_[0];
    if (b.srcRGB == srcRGB && b.dstRGB == dstRGB &&
        b.srcAlpha == srcAlpha && b.dstAlpha == dstAlpha)
        return false;
    b.srcRGB = srcRGB;
    b.dstRGB = dstRGB;
    b.srcAlpha = srcAlpha;
    b.dstAlpha = dstAlpha;
    return true;
}

bool GLStateTracker::setBlendEquation(GLenum mode) {
    BlendState& b = blendBuf_[0];
    if (b.equationRGB == mode && b.equationAlpha == mode) return false;
    b.equationRGB = b.equationAlpha = mode;
    return true;
}

bool GLStateTracker::setBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    BlendState& b = blendBuf_[0];
    if (b.equationRGB == modeRGB && b.equationAlpha == modeAlpha)
        return false;
    b.equationRGB = modeRGB;
    b.equationAlpha = modeAlpha;
    return true;
}

bool GLStateTracker::setBlendFuncSeparatei(uint32_t buf, GLenum srcRGB,
                                           GLenum dstRGB, GLenum srcAlpha,
                                           GLenum dstAlpha) {
    if (buf >= kMaxDrawBuffers) return false; // caller raised error
    BlendState& b = blendBuf_[buf];
    if (b.srcRGB == srcRGB && b.dstRGB == dstRGB &&
        b.srcAlpha == srcAlpha && b.dstAlpha == dstAlpha)
        return false;
    b.srcRGB = srcRGB;
    b.dstRGB = dstRGB;
    b.srcAlpha = srcAlpha;
    b.dstAlpha = dstAlpha;
    return true;
}

bool GLStateTracker::setBlendEquationSeparatei(uint32_t buf, GLenum modeRGB,
                                               GLenum modeAlpha) {
    if (buf >= kMaxDrawBuffers) return false; // caller raised error
    BlendState& b = blendBuf_[buf];
    if (b.equationRGB == modeRGB && b.equationAlpha == modeAlpha)
        return false;
    b.equationRGB = modeRGB;
    b.equationAlpha = modeAlpha;
    return true;
}

bool GLStateTracker::setBlendColor(float r, float g, float b, float a) {
    if (blendColor_.r == r && blendColor_.g == g && blendColor_.b == b &&
        blendColor_.a == a)
        return false;
    blendColor_.r = r;
    blendColor_.g = g;
    blendColor_.b = b;
    blendColor_.a = a;
    return true;
}

bool GLStateTracker::setDepthFunc(GLenum func) {
    if (depth_.func == func) return false;
    depth_.func = func;
    return true;
}

bool GLStateTracker::setDepthMask(bool enabled) {
    if (depth_.mask == enabled) return false;
    depth_.mask = enabled;
    return true;
}

bool GLStateTracker::setDepthRange(double nearVal, double farVal) {
    return setDepthRangeIndexed(0, nearVal, farVal);
}

bool GLStateTracker::setDepthRangeIndexed(uint32_t index, double nearVal,
                                          double farVal) {
    if (index >= kMaxViewports) return false; // caller raised error
    DepthRangeState& d = depthRange_[index];
    if (d.nearVal == nearVal && d.farVal == farVal) return false;
    d.nearVal = nearVal;
    d.farVal = farVal;
    return true;
}

bool GLStateTracker::setStencilFunc(GLenum func, GLint ref, GLuint mask) {
    bool changed = false;
    if (!(stencilFront_.func == func && stencilFront_.ref == ref &&
          stencilFront_.mask == mask)) {
        stencilFront_.func = func;
        stencilFront_.ref = ref;
        stencilFront_.mask = mask;
        changed = true;
    }
    if (!(stencilBack_.func == func && stencilBack_.ref == ref &&
          stencilBack_.mask == mask)) {
        stencilBack_.func = func;
        stencilBack_.ref = ref;
        stencilBack_.mask = mask;
        changed = true;
    }
    return changed;
}

bool GLStateTracker::setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) {
    bool changed = false;
    if (!(stencilFront_.sfail == sfail && stencilFront_.dpfail == dpfail &&
          stencilFront_.dppass == dppass)) {
        stencilFront_.sfail = sfail;
        stencilFront_.dpfail = dpfail;
        stencilFront_.dppass = dppass;
        changed = true;
    }
    if (!(stencilBack_.sfail == sfail && stencilBack_.dpfail == dpfail &&
          stencilBack_.dppass == dppass)) {
        stencilBack_.sfail = sfail;
        stencilBack_.dpfail = dpfail;
        stencilBack_.dppass = dppass;
        changed = true;
    }
    return changed;
}

bool GLStateTracker::setStencilMask(GLuint mask) {
    bool changed = false;
    if (stencilFront_.writeMask != mask) {
        stencilFront_.writeMask = mask;
        changed = true;
    }
    if (stencilBack_.writeMask != mask) {
        stencilBack_.writeMask = mask;
        changed = true;
    }
    return changed;
}

bool GLStateTracker::setStencilFuncSeparate(GLenum face, GLenum func, GLint ref,
                                            GLuint mask) {
    bool changed = false;
    auto apply = [&](StencilFaceState& s) {
        if (!(s.func == func && s.ref == ref && s.mask == mask)) {
            s.func = func;
            s.ref = ref;
            s.mask = mask;
            changed = true;
        }
    };
    if (face == GL_FRONT)
        apply(stencilFront_);
    else if (face == GL_BACK)
        apply(stencilBack_);
    else if (face == GL_FRONT_AND_BACK) {
        apply(stencilFront_);
        apply(stencilBack_);
    }
    return changed;
}

bool GLStateTracker::setStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail,
                                         GLenum dppass) {
    bool changed = false;
    auto apply = [&](StencilFaceState& s) {
        if (!(s.sfail == sfail && s.dpfail == dpfail && s.dppass == dppass)) {
            s.sfail = sfail;
            s.dpfail = dpfail;
            s.dppass = dppass;
            changed = true;
        }
    };
    if (face == GL_FRONT)
        apply(stencilFront_);
    else if (face == GL_BACK)
        apply(stencilBack_);
    else if (face == GL_FRONT_AND_BACK) {
        apply(stencilFront_);
        apply(stencilBack_);
    }
    return changed;
}

bool GLStateTracker::setStencilMaskSeparate(GLenum face, GLuint mask) {
    bool changed = false;
    auto apply = [&](StencilFaceState& s) {
        if (s.writeMask != mask) {
            s.writeMask = mask;
            changed = true;
        }
    };
    if (face == GL_FRONT)
        apply(stencilFront_);
    else if (face == GL_BACK)
        apply(stencilBack_);
    else if (face == GL_FRONT_AND_BACK) {
        apply(stencilFront_);
        apply(stencilBack_);
    }
    return changed;
}

bool GLStateTracker::setColorMask(bool r, bool g, bool b, bool a) {
    bool changed = false;
    for (uint32_t i = 0; i < kMaxDrawBuffers; ++i) {
        ColorMaskState& c = colorMask_[i];
        if (c.r != r || c.g != g || c.b != b || c.a != a) {
            c.r = r;
            c.g = g;
            c.b = b;
            c.a = a;
            changed = true;
        }
    }
    return changed;
}

bool GLStateTracker::setColorMaski(uint32_t buf, bool r, bool g, bool b,
                                   bool a) {
    if (buf >= kMaxDrawBuffers) return false; // caller raised error
    ColorMaskState& c = colorMask_[buf];
    if (c.r == r && c.g == g && c.b == b && c.a == a) return false;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return true;
}

bool GLStateTracker::setSampleCoverage(float value, bool invert) {
    if (sampleCoverage_.value == value && sampleCoverage_.invert == invert)
        return false;
    sampleCoverage_.value = value;
    sampleCoverage_.invert = invert;
    return true;
}

bool GLStateTracker::setPrimitiveRestartIndex(uint32_t index) {
    if (primitiveRestart_.index == index) return false;
    primitiveRestart_.index = index;
    return true;
}

bool GLStateTracker::setCullFace(GLenum mode) {
    if (raster_.cull == mode) return false;
    raster_.cull = mode;
    return true;
}

bool GLStateTracker::setFrontFace(GLenum mode) {
    if (raster_.front == mode) return false;
    raster_.front = mode;
    return true;
}

bool GLStateTracker::setPointSize(float size) {
    if (rasterScalar_.pointSize == size) return false;
    rasterScalar_.pointSize = size;
    return true;
}

bool GLStateTracker::setLineWidth(float width) {
    if (rasterScalar_.lineWidth == width) return false;
    rasterScalar_.lineWidth = width;
    return true;
}

bool GLStateTracker::setPolygonOffset(float factor, float units) {
    if (rasterScalar_.polygonOffsetFactor == factor &&
        rasterScalar_.polygonOffsetUnits == units)
        return false;
    rasterScalar_.polygonOffsetFactor = factor;
    rasterScalar_.polygonOffsetUnits = units;
    return true;
}

bool GLStateTracker::setPolygonOffsetClamp(float factor, float units,
                                           float clamp) {
    if (rasterScalar_.polygonOffsetFactor == factor &&
        rasterScalar_.polygonOffsetUnits == units &&
        rasterScalar_.polygonOffsetClamp == clamp)
        return false;
    rasterScalar_.polygonOffsetFactor = factor;
    rasterScalar_.polygonOffsetUnits = units;
    rasterScalar_.polygonOffsetClamp = clamp;
    return true;
}

bool GLStateTracker::setPointParameteri(GLenum pname, GLint param) {
    switch (pname) {
        case GL_POINT_SIZE_MIN:
            pointParam_.sizeMin = static_cast<float>(param); break;
        case GL_POINT_SIZE_MAX:
            pointParam_.sizeMax = static_cast<float>(param); break;
        case GL_POINT_FADE_THRESHOLD_SIZE:
            pointParam_.fadeThreshold = static_cast<float>(param); break;
        case GL_POINT_SPRITE_COORD_ORIGIN:
            pointParam_.spriteCoordOrigin = static_cast<GLenum>(param); break;
        default: return false;
    }
    return true;
}

bool GLStateTracker::setPointParameterf(GLenum pname, GLfloat param) {
    switch (pname) {
        case GL_POINT_SIZE_MIN:
            pointParam_.sizeMin = param; break;
        case GL_POINT_SIZE_MAX:
            pointParam_.sizeMax = param; break;
        case GL_POINT_FADE_THRESHOLD_SIZE:
            pointParam_.fadeThreshold = param; break;
        case GL_POINT_SPRITE_COORD_ORIGIN:
            pointParam_.spriteCoordOrigin = static_cast<GLenum>(static_cast<int>(param)); break;
        default: return false;
    }
    return true;
}

bool GLStateTracker::setPointParameteriv(GLenum pname, const GLint* params) {
    if (params == nullptr) return false;
    return setPointParameteri(pname, params[0]);
}

bool GLStateTracker::setPointParameterfv(GLenum pname, const GLfloat* params) {
    if (params == nullptr) return false;
    return setPointParameterf(pname, params[0]);
}

bool GLStateTracker::setPatchParameteri(GLenum pname, GLint value) {
    if (pname != GL_PATCH_VERTICES) return false;
    uint32_t v = static_cast<uint32_t>(value);
    bool changed = (v != patch_.patchVertices);
    patch_.patchVertices = v;
    return changed;
}

bool GLStateTracker::setPatchParameterfv(GLenum pname, const GLfloat* values) {
    if (values == nullptr) return false;
    if (pname == GL_PATCH_DEFAULT_OUTER_LEVEL) {
        std::array<float, 4> v;
        for (int i = 0; i < 4; ++i) v[i] = values[i];
        bool changed = (v != patch_.patchOuterLevel);
        patch_.patchOuterLevel = v;
        return changed;
    }
    if (pname == GL_PATCH_DEFAULT_INNER_LEVEL) {
        std::array<float, 2> v;
        for (int i = 0; i < 2; ++i) v[i] = values[i];
        bool changed = (v != patch_.patchInnerLevel);
        patch_.patchInnerLevel = v;
        return changed;
    }
    return false;
}

bool GLStateTracker::setClipControl(GLenum origin, GLenum depth) {
    clip_.origin = origin;
    clip_.depth = depth;
    return true;
}

bool GLStateTracker::setPolygonMode(GLenum face, GLenum mode) {
    const bool front = (face == 0x0404 /* GL_FRONT */) ||
                       (face == 0x0408 /* GL_FRONT_AND_BACK */);
    const bool back = (face == 0x0405 /* GL_BACK */) ||
                      (face == 0x0408 /* GL_FRONT_AND_BACK */);
    const bool validMode = (mode == 0x1B00 /* GL_POINT */ ||
                            mode == 0x1B01 /* GL_LINE */ ||
                            mode == 0x1B02 /* GL_FILL */);
    if ((!front && !back) || !validMode) return false; // caller raised error
    bool changed = false;
    if (front && polygonMode_.front != mode) {
        polygonMode_.front = mode;
        changed = true;
    }
    if (back && polygonMode_.back != mode) {
        polygonMode_.back = mode;
        changed = true;
    }
    return changed;
}

bool GLStateTracker::setSampleMaski(GLuint maskNumber, GLuint mask) {
    if (maskNumber >= kMaxSampleMaskWords) return false; // caller raised error
    if (multisampleRaster_.sampleMask[maskNumber] == mask) return false;
    multisampleRaster_.sampleMask[maskNumber] = mask;
    return true;
}

bool GLStateTracker::setMinSampleShading(float value) {
    if (multisampleRaster_.minSampleShading == value) return false;
    multisampleRaster_.minSampleShading = value;
    return true;
}

bool GLStateTracker::setProvokingVertex(GLenum mode) {
    if (provokingVertex_.mode == mode) return false;
    provokingVertex_.mode = mode;
    return true;
}

bool GLStateTracker::setClampColor(GLenum target, GLenum mode) {
    if (target != 0x891C /* GL_CLAMP_READ_COLOR */) return false;
    if (mode != GL_TRUE && mode != GL_FALSE && mode != 0x891D /* GL_FIXED_ONLY */)
        return false;
    if (clampColor_.readColor == mode) return false;
    clampColor_.readColor = mode;
    return true;
}

bool GLStateTracker::setPixelStorei(GLenum pname, GLint param) {
    GLint* field = nullptr;
    switch (pname) {
    case GL_PACK_SWAP_BYTES: field = &pixel_.packSwapBytes; break;
    case GL_PACK_LSB_FIRST: field = &pixel_.packLsbFirst; break;
    case GL_PACK_ROW_LENGTH: field = &pixel_.packRowLength; break;
    case GL_PACK_IMAGE_HEIGHT: field = &pixel_.packImageHeight; break;
    case GL_PACK_SKIP_ROW: field = &pixel_.packSkipRow; break;
    case GL_PACK_SKIP_PIXELS: field = &pixel_.packSkipPixels; break;
    case GL_PACK_ALIGNMENT: field = &pixel_.packAlignment; break;
    case GL_PACK_SKIP_IMAGES: field = &pixel_.packSkipImages; break;
    case GL_PACK_COMPRESSED_BLOCK_WIDTH: field = &pixel_.packCompressedBlockWidth; break;
    case GL_PACK_COMPRESSED_BLOCK_HEIGHT: field = &pixel_.packCompressedBlockHeight; break;
    case GL_PACK_COMPRESSED_BLOCK_DEPTH: field = &pixel_.packCompressedBlockDepth; break;
    case GL_PACK_COMPRESSED_BLOCK_SIZE: field = &pixel_.packCompressedBlockSize; break;
    case GL_UNPACK_SWAP_BYTES: field = &pixel_.unpackSwapBytes; break;
    case GL_UNPACK_LSB_FIRST: field = &pixel_.unpackLsbFirst; break;
    case GL_UNPACK_ROW_LENGTH: field = &pixel_.unpackRowLength; break;
    case GL_UNPACK_IMAGE_HEIGHT: field = &pixel_.unpackImageHeight; break;
    case GL_UNPACK_SKIP_ROW: field = &pixel_.unpackSkipRow; break;
    case GL_UNPACK_SKIP_PIXELS: field = &pixel_.unpackSkipPixels; break;
    case GL_UNPACK_ALIGNMENT: field = &pixel_.unpackAlignment; break;
    case GL_UNPACK_SKIP_IMAGES: field = &pixel_.unpackSkipImages; break;
    case GL_UNPACK_COMPRESSED_BLOCK_WIDTH: field = &pixel_.unpackCompressedBlockWidth; break;
    case GL_UNPACK_COMPRESSED_BLOCK_HEIGHT: field = &pixel_.unpackCompressedBlockHeight; break;
    case GL_UNPACK_COMPRESSED_BLOCK_DEPTH: field = &pixel_.unpackCompressedBlockDepth; break;
    case GL_UNPACK_COMPRESSED_BLOCK_SIZE: field = &pixel_.unpackCompressedBlockSize; break;
    default: return false; // not a recognized pixel-store pname
    }
    if (*field == param) return false;
    *field = param;
    return true;
}

bool GLStateTracker::getPixelStorei(GLenum pname, GLint* out) const {
    switch (pname) {
    case GL_PACK_SWAP_BYTES: *out = pixel_.packSwapBytes; return true;
    case GL_PACK_LSB_FIRST: *out = pixel_.packLsbFirst; return true;
    case GL_PACK_ROW_LENGTH: *out = pixel_.packRowLength; return true;
    case GL_PACK_IMAGE_HEIGHT: *out = pixel_.packImageHeight; return true;
    case GL_PACK_SKIP_ROW: *out = pixel_.packSkipRow; return true;
    case GL_PACK_SKIP_PIXELS: *out = pixel_.packSkipPixels; return true;
    case GL_PACK_ALIGNMENT: *out = pixel_.packAlignment; return true;
    case GL_PACK_SKIP_IMAGES: *out = pixel_.packSkipImages; return true;
    case GL_PACK_COMPRESSED_BLOCK_WIDTH: *out = pixel_.packCompressedBlockWidth; return true;
    case GL_PACK_COMPRESSED_BLOCK_HEIGHT: *out = pixel_.packCompressedBlockHeight; return true;
    case GL_PACK_COMPRESSED_BLOCK_DEPTH: *out = pixel_.packCompressedBlockDepth; return true;
    case GL_PACK_COMPRESSED_BLOCK_SIZE: *out = pixel_.packCompressedBlockSize; return true;
    case GL_UNPACK_SWAP_BYTES: *out = pixel_.unpackSwapBytes; return true;
    case GL_UNPACK_LSB_FIRST: *out = pixel_.unpackLsbFirst; return true;
    case GL_UNPACK_ROW_LENGTH: *out = pixel_.unpackRowLength; return true;
    case GL_UNPACK_IMAGE_HEIGHT: *out = pixel_.unpackImageHeight; return true;
    case GL_UNPACK_SKIP_ROW: *out = pixel_.unpackSkipRow; return true;
    case GL_UNPACK_SKIP_PIXELS: *out = pixel_.unpackSkipPixels; return true;
    case GL_UNPACK_ALIGNMENT: *out = pixel_.unpackAlignment; return true;
    case GL_UNPACK_SKIP_IMAGES: *out = pixel_.unpackSkipImages; return true;
    case GL_UNPACK_COMPRESSED_BLOCK_WIDTH: *out = pixel_.unpackCompressedBlockWidth; return true;
    case GL_UNPACK_COMPRESSED_BLOCK_HEIGHT: *out = pixel_.unpackCompressedBlockHeight; return true;
    case GL_UNPACK_COMPRESSED_BLOCK_DEPTH: *out = pixel_.unpackCompressedBlockDepth; return true;
    case GL_UNPACK_COMPRESSED_BLOCK_SIZE: *out = pixel_.unpackCompressedBlockSize; return true;
    default: return false;
    }
}

bool GLStateTracker::setViewport(GLint x, GLint y, GLsizei width,
                                   GLsizei height) {
    return setViewportIndexed(0, x, y, width, height);
}

bool GLStateTracker::setViewportIndexed(GLuint index, GLint x, GLint y,
                                         GLsizei width, GLsizei height) {
    if (index >= kMaxViewports) return false;
    ViewportState& v = viewport_[index];
    if (v.x == x && v.y == y && v.width == width && v.height == height)
        return false;
    v.x = x;
    v.y = y;
    v.width = width;
    v.height = height;
    return true;
}

bool GLStateTracker::setScissor(GLint x, GLint y, GLsizei width,
                                 GLsizei height) {
    return setScissorIndexed(0, x, y, width, height);
}

bool GLStateTracker::setScissorIndexed(GLuint index, GLint x, GLint y,
                                        GLsizei width, GLsizei height) {
    if (index >= kMaxViewports) return false;
    ScissorBoxState& s = scissor_[index];
    if (s.x == x && s.y == y && s.width == width && s.height == height)
        return false;
    s.x = x;
    s.y = y;
    s.width = width;
    s.height = height;
    return true;
}

bool GLStateTracker::setViewportIndexedv(uint32_t first, uint32_t count,
                                         const GLfloat* v) {
    if (v == nullptr) return false;
    bool changed = false;
    for (uint32_t i = 0; i < count; ++i) {
        changed |= setViewportIndexed(first + i,
                                      static_cast<GLint>(v[4 * i + 0]),
                                      static_cast<GLint>(v[4 * i + 1]),
                                      static_cast<GLsizei>(v[4 * i + 2]),
                                      static_cast<GLsizei>(v[4 * i + 3]));
    }
    return changed;
}

bool GLStateTracker::setScissorIndexedv(uint32_t first, uint32_t count,
                                        const GLint* v) {
    if (v == nullptr) return false;
    bool changed = false;
    for (uint32_t i = 0; i < count; ++i) {
        changed |= setScissorIndexed(first + i, v[4 * i + 0], v[4 * i + 1],
                                     v[4 * i + 2], v[4 * i + 3]);
    }
    return changed;
}

bool GLStateTracker::setClearColor(float r, float g, float b, float a) {
    if (clearColor_.r == r && clearColor_.g == g && clearColor_.b == b &&
        clearColor_.a == a)
        return false;
    clearColor_.r = r;
    clearColor_.g = g;
    clearColor_.b = b;
    clearColor_.a = a;
    return true;
}

bool GLStateTracker::setClearDepth(double d) {
    if (clearDepth_.depth == d) return false;
    clearDepth_.depth = d;
    return true;
}

bool GLStateTracker::setClearStencil(int s) {
    if (clearStencil_.stencil == s) return false;
    clearStencil_.stencil = s;
    return true;
}

bool GLStateTracker::setDrawBuffers(const std::vector<GLenum>& bufs) {
    if (fbBuffers_.draw == bufs) return false;
    fbBuffers_.draw = bufs;
    return true;
}

bool GLStateTracker::setReadBuffer(GLenum buf) {
    if (fbBuffers_.read == buf) return false;
    fbBuffers_.read = buf;
    return true;
}

bool GLStateTracker::setLogicOp(GLenum mode) {
    if (logicOp_.mode == mode) return false;
    logicOp_.mode = mode;
    return true;
}

bool GLStateTracker::setActiveTexture(GLenum texture) {
    if (texture < GL_TEXTURE0) return false; // not a texture-unit enum
    uint32_t unit = texture - GL_TEXTURE0;
    if (unit >= kMaxTextureUnits) return false; // out of range
    if (activeTextureUnit_ == unit) return false;
    activeTextureUnit_ = unit;
    textureUnitsDirty_ = true; // active unit must be re-pushed to the driver
    return true;
}

bool GLStateTracker::setTextureBinding(GLenum target, GLObjectName name) {
    auto& bound = texUnits_[activeTextureUnit_].bound;
    auto it = bound.find(target);
    if (it != bound.end() && it->second == name) return false;
    bound[target] = name;
    textureUnitsDirty_ = true;
    return true;
}

GLenum normalizeTextureTarget(GLenum target) {
    switch (target) {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return GL_TEXTURE_CUBE_MAP;
    default:
        return target;
    }
}

GLObjectName GLStateTracker::boundTextureForTarget(GLenum target) const {
    if (activeTextureUnit_ >= texUnits_.size()) return 0;
    const auto& bound = texUnits_[activeTextureUnit_].bound;
    auto it = bound.find(normalizeTextureTarget(target));
    return it == bound.end() ? 0 : it->second;
}

bool GLStateTracker::setTextureUnitBinding(uint32_t unit, GLenum target,
                                           GLObjectName name) {
    if (unit >= kMaxTextureUnits) return false; // out of range
    auto& bound = texUnits_[unit].bound;
    auto it = bound.find(target);
    if (it != bound.end() && it->second == name) return false;
    if (name == 0) {
        // glBindTextureUnit(unit, 0) unbinds the whole unit (default texture).
        if (bound.empty()) return false;
        bound.clear();
    } else {
        bound[target] = name;
    }
    textureUnitsDirty_ = true;
    return true;
}

GLObjectName GLStateTracker::boundTextureForUnitTarget(uint32_t unit,
                                                       GLenum target) const {
    if (unit >= texUnits_.size()) return 0;
    const auto& bound = texUnits_[unit].bound;
    auto it = bound.find(target);
    return it == bound.end() ? 0 : it->second;
}

bool GLStateTracker::clearTextureBinding(GLObjectName name) {
    bool changed = false;
    for (auto& unit : texUnits_) {
        for (auto& kv : unit.bound) {
            if (kv.second == name) {
                kv.second = 0;
                changed = true;
            }
        }
    }
    if (changed) textureUnitsDirty_ = true;
    return changed;
}

bool GLStateTracker::setSamplerBinding(uint32_t unit, GLObjectName name) {
    if (unit >= kMaxTextureUnits) return false;
    if (samplerBound_[unit] == name) return false;
    samplerBound_[unit] = name;
    samplerUnitsDirty_ = true;
    return true;
}

GLObjectName GLStateTracker::boundSamplerForUnit(uint32_t unit) const {
    if (unit >= samplerBound_.size()) return 0;
    return samplerBound_[unit];
}

bool GLStateTracker::clearSamplerBinding(GLObjectName name) {
    bool changed = false;
    for (auto& s : samplerBound_) {
        if (s == name) {
            s = 0;
            changed = true;
        }
    }
    if (changed) samplerUnitsDirty_ = true;
    return changed;
}

bool GLStateTracker::setSamplerBindings(uint32_t first, uint32_t count,
                                        const GLObjectName* names) {
    if (first > kMaxTextureUnits || first + count > kMaxTextureUnits)
        return false; // out of range
    bool changed = false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t unit = first + i;
        GLObjectName name = (names != nullptr) ? names[i] : 0;
        if (samplerBound_[unit] != name) {
            samplerBound_[unit] = name;
            changed = true;
        }
    }
    if (changed) samplerUnitsDirty_ = true;
    return changed;
}

bool GLStateTracker::setImageUnitBinding(uint32_t unit, GLObjectName texture,
                                         GLint level, bool layered, GLint layer,
                                         GLenum access, GLenum format) {
    if (unit >= kMaxImageUnits) return false; // out of range
    ImageUnitBinding b;
    b.texture = texture;
    b.level = level;
    b.layered = layered;
    b.layer = layer;
    b.access = access;
    b.format = format;
    if (imageUnit_[unit].equal(b)) return false;
    imageUnit_[unit] = b;
    imageUnitsDirty_ = true;
    return true;
}

bool GLStateTracker::setImageUnitBindings(uint32_t first, uint32_t count,
                                          const GLObjectName* names) {
    if (first > kMaxImageUnits || first + count > kMaxImageUnits)
        return false; // out of range
    bool changed = false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t unit = first + i;
        GLObjectName name = (names != nullptr) ? names[i] : 0;
        ImageUnitBinding b;
        b.texture = name;
        b.level = 0;
        b.layered = false;
        b.layer = 0;
        b.access = GL_READ_ONLY;
        b.format = GL_RGBA32F;
        if (!imageUnit_[unit].equal(b)) {
            imageUnit_[unit] = b;
            changed = true;
        }
    }
    if (changed) imageUnitsDirty_ = true;
    return changed;
}

GLObjectName GLStateTracker::boundImageTextureForUnit(uint32_t unit) const {
    if (unit >= imageUnit_.size()) return 0;
    return imageUnit_[unit].texture;
}

int GLStateTracker::apply(GLStateSink& sink) {
    int applied = 0;

    if (capsDirty_) {
        for (const auto& kv : capsCurrent_) {
            auto appliedIt = capsApplied_.find(kv.first);
            if (appliedIt == capsApplied_.end() ||
                appliedIt->second != kv.second) {
                if (kv.second)
                    sink.enable(kv.first);
                else
                    sink.disable(kv.first);
            }
        }
        capsApplied_ = capsCurrent_;
        capsDirty_ = false;
        ++applied;
    }

    if (indexedCapsDirty_) {
        for (const auto& capEntry : indexedCapsCurrent_) {
            auto& appliedMap = indexedCapsApplied_[capEntry.first];
            for (const auto& kv : capEntry.second) {
                auto appliedIt = appliedMap.find(kv.first);
                if (appliedIt == appliedMap.end() ||
                    appliedIt->second != kv.second) {
                    if (kv.second)
                        sink.enableIndexed(capEntry.first, kv.first);
                    else
                        sink.disableIndexed(capEntry.first, kv.first);
                    appliedMap[kv.first] = kv.second;
                }
            }
        }
        indexedCapsDirty_ = false;
        ++applied;
    }

    if (hintsDirty_) {
        for (const auto& kv : hints_) {
            auto appliedIt = hintsApplied_.find(kv.first);
            if (appliedIt == hintsApplied_.end() ||
                appliedIt->second != kv.second) {
                sink.hint(kv.first, kv.second);
            }
        }
        hintsApplied_ = hints_;
        hintsDirty_ = false;
        ++applied;
    }

    if (programDirty_) {
        sink.useProgram(activeProgram_);
        activeProgramApplied_ = activeProgram_;
        programDirty_ = false;
        ++applied;
    }

    if (programPipelineDirty_) {
        sink.bindProgramPipeline(boundProgramPipeline_);
        boundProgramPipelineApplied_ = boundProgramPipeline_;
        programPipelineDirty_ = false;
        ++applied;
    }

    for (uint32_t i = 0; i < kMaxDrawBuffers; ++i) {
        if (blendBuf_[i].equal(blendBufApplied_[i])) continue;
        if (i == 0) {
            // Buffer 0 is the non-indexed blend path: push through the single-
            // buffer sink methods so backends without per-buffer blend stay
            // compatible.
            sink.blendFuncSeparate(blendBuf_[0].srcRGB, blendBuf_[0].dstRGB,
                                   blendBuf_[0].srcAlpha, blendBuf_[0].dstAlpha);
            sink.blendEquationSeparate(blendBuf_[0].equationRGB,
                                       blendBuf_[0].equationAlpha);
        } else {
            sink.blendFuncSeparatei(i, blendBuf_[i].srcRGB, blendBuf_[i].dstRGB,
                                    blendBuf_[i].srcAlpha, blendBuf_[i].dstAlpha);
            sink.blendEquationSeparatei(i, blendBuf_[i].equationRGB,
                                        blendBuf_[i].equationAlpha);
        }
        blendBufApplied_[i] = blendBuf_[i];
        ++applied;
    }

    if (!blendColor_.equal(blendColorApplied_)) {
        sink.blendColor(blendColor_.r, blendColor_.g, blendColor_.b, blendColor_.a);
        blendColorApplied_ = blendColor_;
        ++applied;
    }

    if (!depth_.equal(depthApplied_)) {
        sink.depthFunc(depth_.func);
        sink.depthMask(depth_.mask);
        depthApplied_ = depth_;
        ++applied;
    }

    for (uint32_t i = 0; i < kMaxViewports; ++i) {
        if (!depthRange_[i].equal(depthRangeApplied_[i])) {
            if (i == 0)
                sink.depthRange(depthRange_[i].nearVal, depthRange_[i].farVal);
            else
                sink.depthRangeIndexed(i, depthRange_[i].nearVal,
                                       depthRange_[i].farVal);
            depthRangeApplied_[i] = depthRange_[i];
            ++applied;
        }
    }

    // Stencil: push per-face. When both faces are equal and changed, a single
    // both-faces call (stencilFunc/Op/Mask) is sufficient (SPEC §10); otherwise
    // push each differing face via the *Separate sink methods.
    if (stencilFront_.equal(stencilBack_)) {
        if (!stencilFront_.equal(stencilFrontApplied_)) {
            sink.stencilFunc(stencilFront_.func, stencilFront_.ref, stencilFront_.mask);
            sink.stencilOp(stencilFront_.sfail, stencilFront_.dpfail,
                           stencilFront_.dppass);
            sink.stencilMask(stencilFront_.writeMask);
            stencilFrontApplied_ = stencilBackApplied_ = stencilFront_;
            ++applied;
        }
    } else {
        if (!stencilFront_.equal(stencilFrontApplied_)) {
            sink.stencilFuncSeparate(GL_FRONT, stencilFront_.func, stencilFront_.ref,
                                     stencilFront_.mask);
            sink.stencilOpSeparate(GL_FRONT, stencilFront_.sfail, stencilFront_.dpfail,
                                   stencilFront_.dppass);
            sink.stencilMaskSeparate(GL_FRONT, stencilFront_.writeMask);
            stencilFrontApplied_ = stencilFront_;
            ++applied;
        }
        if (!stencilBack_.equal(stencilBackApplied_)) {
            sink.stencilFuncSeparate(GL_BACK, stencilBack_.func, stencilBack_.ref,
                                     stencilBack_.mask);
            sink.stencilOpSeparate(GL_BACK, stencilBack_.sfail, stencilBack_.dpfail,
                                   stencilBack_.dppass);
            sink.stencilMaskSeparate(GL_BACK, stencilBack_.writeMask);
            stencilBackApplied_ = stencilBack_;
            ++applied;
        }
    }

    if (!raster_.equal(rasterApplied_)) {
        sink.cullFace(raster_.cull);
        sink.frontFace(raster_.front);
        rasterApplied_ = raster_;
        ++applied;
    }

    if (!rasterScalar_.equal(rasterScalarApplied_)) {
        if (rasterScalar_.pointSize != rasterScalarApplied_.pointSize)
            sink.pointSize(rasterScalar_.pointSize);
        if (rasterScalar_.lineWidth != rasterScalarApplied_.lineWidth)
            sink.lineWidth(rasterScalar_.lineWidth);
        if (rasterScalar_.polygonOffsetFactor !=
                rasterScalarApplied_.polygonOffsetFactor ||
            rasterScalar_.polygonOffsetUnits !=
                rasterScalarApplied_.polygonOffsetUnits ||
            rasterScalar_.polygonOffsetClamp !=
                rasterScalarApplied_.polygonOffsetClamp)
            sink.polygonOffset(rasterScalar_.polygonOffsetFactor,
                               rasterScalar_.polygonOffsetUnits,
                               rasterScalar_.polygonOffsetClamp);
        rasterScalarApplied_ = rasterScalar_;
        ++applied;
    }

    if (!pointParam_.equal(pointParamApplied_)) {
        sink.pointParameters(pointParam_.sizeMin, pointParam_.sizeMax,
                             pointParam_.fadeThreshold,
                             pointParam_.spriteCoordOrigin);
        pointParamApplied_ = pointParam_;
        ++applied;
    }

    if (!patch_.equal(patchApplied_)) {
        if (patch_.patchVertices != patchApplied_.patchVertices) {
            sink.patchParameteri(GL_PATCH_VERTICES,
                                 static_cast<int>(patch_.patchVertices));
        }
        if (patch_.patchOuterLevel != patchApplied_.patchOuterLevel) {
            sink.patchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL,
                                  patch_.patchOuterLevel.data());
        }
        if (patch_.patchInnerLevel != patchApplied_.patchInnerLevel) {
            sink.patchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL,
                                  patch_.patchInnerLevel.data());
        }
        patchApplied_ = patch_;
        ++applied;
    }

    if (!clip_.equal(clipApplied_)) {
        sink.clipControl(clip_.origin, clip_.depth);
        clipApplied_ = clip_;
        ++applied;
    }

    if (!pixel_.equal(pixelApplied_)) {
        // Push only the individual pixel-store parameters that changed (SPEC §10:
        // avoid redundant native glPixelStorei calls). The backend interprets each
        // pname independently.
#define YAGLT_PUSH_PIXEL(field, pname)                          \
        if (pixel_.field != pixelApplied_.field) {             \
            sink.pixelStorei(pname, pixel_.field);              \
            pixelApplied_.field = pixel_.field;                 \
        }
        YAGLT_PUSH_PIXEL(packSwapBytes, GL_PACK_SWAP_BYTES)
        YAGLT_PUSH_PIXEL(packLsbFirst, GL_PACK_LSB_FIRST)
        YAGLT_PUSH_PIXEL(packRowLength, GL_PACK_ROW_LENGTH)
        YAGLT_PUSH_PIXEL(packImageHeight, GL_PACK_IMAGE_HEIGHT)
        YAGLT_PUSH_PIXEL(packSkipRow, GL_PACK_SKIP_ROW)
        YAGLT_PUSH_PIXEL(packSkipPixels, GL_PACK_SKIP_PIXELS)
        YAGLT_PUSH_PIXEL(packAlignment, GL_PACK_ALIGNMENT)
        YAGLT_PUSH_PIXEL(packSkipImages, GL_PACK_SKIP_IMAGES)
        YAGLT_PUSH_PIXEL(packCompressedBlockWidth, GL_PACK_COMPRESSED_BLOCK_WIDTH)
        YAGLT_PUSH_PIXEL(packCompressedBlockHeight, GL_PACK_COMPRESSED_BLOCK_HEIGHT)
        YAGLT_PUSH_PIXEL(packCompressedBlockDepth, GL_PACK_COMPRESSED_BLOCK_DEPTH)
        YAGLT_PUSH_PIXEL(packCompressedBlockSize, GL_PACK_COMPRESSED_BLOCK_SIZE)
        YAGLT_PUSH_PIXEL(unpackSwapBytes, GL_UNPACK_SWAP_BYTES)
        YAGLT_PUSH_PIXEL(unpackLsbFirst, GL_UNPACK_LSB_FIRST)
        YAGLT_PUSH_PIXEL(unpackRowLength, GL_UNPACK_ROW_LENGTH)
        YAGLT_PUSH_PIXEL(unpackImageHeight, GL_UNPACK_IMAGE_HEIGHT)
        YAGLT_PUSH_PIXEL(unpackSkipRow, GL_UNPACK_SKIP_ROW)
        YAGLT_PUSH_PIXEL(unpackSkipPixels, GL_UNPACK_SKIP_PIXELS)
        YAGLT_PUSH_PIXEL(unpackAlignment, GL_UNPACK_ALIGNMENT)
        YAGLT_PUSH_PIXEL(unpackSkipImages, GL_UNPACK_SKIP_IMAGES)
        YAGLT_PUSH_PIXEL(unpackCompressedBlockWidth, GL_UNPACK_COMPRESSED_BLOCK_WIDTH)
        YAGLT_PUSH_PIXEL(unpackCompressedBlockHeight, GL_UNPACK_COMPRESSED_BLOCK_HEIGHT)
        YAGLT_PUSH_PIXEL(unpackCompressedBlockDepth, GL_UNPACK_COMPRESSED_BLOCK_DEPTH)
        YAGLT_PUSH_PIXEL(unpackCompressedBlockSize, GL_UNPACK_COMPRESSED_BLOCK_SIZE)
#undef YAGLT_PUSH_PIXEL
        ++applied;
    }

    for (uint32_t i = 0; i < kMaxViewports; ++i) {
        if (!viewport_[i].equal(viewportApplied_[i])) {
            if (i == 0)
                sink.setViewport(viewport_[i].x, viewport_[i].y,
                                 viewport_[i].width, viewport_[i].height);
            else
                sink.setViewportIndexed(i, viewport_[i].x, viewport_[i].y,
                                        viewport_[i].width, viewport_[i].height);
            viewportApplied_[i] = viewport_[i];
            ++applied;
        }
        if (!scissor_[i].equal(scissorApplied_[i])) {
            if (i == 0)
                sink.setScissor(scissor_[i].x, scissor_[i].y, scissor_[i].width,
                                scissor_[i].height);
            else
                sink.setScissorIndexed(i, scissor_[i].x, scissor_[i].y,
                                       scissor_[i].width, scissor_[i].height);
            scissorApplied_[i] = scissor_[i];
            ++applied;
        }
    }

    if (!clearColor_.equal(clearColorApplied_)) {
        sink.clearColor(clearColor_.r, clearColor_.g, clearColor_.b,
                        clearColor_.a);
        clearColorApplied_ = clearColor_;
        ++applied;
    }

    if (!clearDepth_.equal(clearDepthApplied_)) {
        sink.clearDepth(clearDepth_.depth);
        clearDepthApplied_ = clearDepth_;
        ++applied;
    }

    if (!clearStencil_.equal(clearStencilApplied_)) {
        sink.clearStencil(clearStencil_.stencil);
        clearStencilApplied_ = clearStencil_;
        ++applied;
    }

    if (!fbBuffers_.equal(fbBuffersApplied_)) {
        if (!fbBuffers_.draw.empty()) {
            sink.drawBuffers(static_cast<int32_t>(fbBuffers_.draw.size()),
                             fbBuffers_.draw.data());
        }
        sink.readBuffer(fbBuffers_.read);
        fbBuffersApplied_ = fbBuffers_;
        ++applied;
    }

    if (!logicOp_.equal(logicOpApplied_)) {
        sink.logicOp(logicOp_.mode);
        logicOpApplied_ = logicOp_;
        ++applied;
    }

    for (uint32_t i = 0; i < kMaxDrawBuffers; ++i) {
        if (colorMask_[i].equal(colorMaskApplied_[i])) continue;
        if (i == 0) {
            // Buffer 0 is the non-indexed color-mask path.
            sink.colorMask(colorMask_[0].r, colorMask_[0].g, colorMask_[0].b,
                           colorMask_[0].a);
        } else {
            sink.colorMaski(i, colorMask_[i].r, colorMask_[i].g,
                            colorMask_[i].b, colorMask_[i].a);
        }
        colorMaskApplied_[i] = colorMask_[i];
        ++applied;
    }

    if (!sampleCoverage_.equal(sampleCoverageApplied_)) {
        sink.sampleCoverage(sampleCoverage_.value, sampleCoverage_.invert);
        sampleCoverageApplied_ = sampleCoverage_;
        ++applied;
    }

    if (!polygonMode_.equal(polygonModeApplied_)) {
        sink.polygonMode(polygonMode_.front, polygonMode_.back);
        polygonModeApplied_ = polygonMode_;
        ++applied;
    }

    if (!multisampleRaster_.equal(multisampleRasterApplied_)) {
        // Push only the mask words that actually changed (SPEC §10): the backend
        // applies each word independently, so a no-op word must not be re-sent.
        for (uint32_t i = 0; i < kMaxSampleMaskWords; ++i) {
            if (multisampleRaster_.sampleMask[i] !=
                multisampleRasterApplied_.sampleMask[i])
                sink.sampleMaski(i, multisampleRaster_.sampleMask[i]);
        }
        if (multisampleRaster_.minSampleShading !=
            multisampleRasterApplied_.minSampleShading)
            sink.minSampleShading(multisampleRaster_.minSampleShading);
        multisampleRasterApplied_ = multisampleRaster_;
        ++applied;
    }

    if (!primitiveRestart_.equal(primitiveRestartApplied_)) {
        sink.primitiveRestart(primitiveRestart_.index);
        primitiveRestartApplied_ = primitiveRestart_;
        ++applied;
    }

    if (!provokingVertex_.equal(provokingVertexApplied_)) {
        sink.provokingVertex(provokingVertex_.mode);
        provokingVertexApplied_ = provokingVertex_;
        ++applied;
    }

    if (!clampColor_.equal(clampColorApplied_)) {
        sink.clampColor(0x891C /* GL_CLAMP_READ_COLOR */, clampColor_.readColor);
        clampColorApplied_ = clampColor_;
        ++applied;
    }

    if (textureUnitsDirty_) {
        // Ensure the driver's active unit matches the frontend's active unit.
        if (activeTextureApplied_ != activeTextureUnit_) {
            sink.activeTexture(GL_TEXTURE0 + activeTextureUnit_);
            activeTextureApplied_ = activeTextureUnit_;
        }
        uint32_t lastUnit = activeTextureApplied_;
        for (uint32_t i = 0; i < texUnits_.size(); ++i) {
            const auto& cur = texUnits_[i].bound;
            const auto& app = texUnitsApplied_[i].bound;
            // Collect every target touched in either the current or applied
            // state so bindings that were removed (reset to 0) are pushed too.
            std::vector<GLenum> keys;
            for (const auto& kv : cur) keys.push_back(kv.first);
            for (const auto& kv : app)
                if (cur.find(kv.first) == cur.end()) keys.push_back(kv.first);
            for (GLenum t : keys) {
                GLObjectName c = cur.count(t) ? cur.at(t) : 0;
                GLObjectName a = app.count(t) ? app.at(t) : 0;
                if (c == a) continue;
                if (i != lastUnit) {
                    sink.activeTexture(GL_TEXTURE0 + i);
                    lastUnit = i;
                }
                sink.bindTexture(t, c);
            }
            texUnitsApplied_[i].bound = cur;
        }
        textureUnitsDirty_ = false;
        ++applied;
    }

    if (samplerUnitsDirty_) {
        for (uint32_t i = 0; i < samplerBound_.size(); ++i) {
            GLObjectName c = samplerBound_[i];
            GLObjectName a = samplerBoundApplied_[i];
            if (c == a) continue;
            // glBindSampler takes the zero-based unit index directly, so the
            // backend resolves `c` (frontend name) to its native id. No active
            // texture switch is required here (SPEC §8.2).
            sink.bindSampler(i, c);
        }
        samplerBoundApplied_ = samplerBound_;
        samplerUnitsDirty_ = false;
        ++applied;
    }

    if (imageUnitsDirty_) {
        for (uint32_t i = 0; i < kMaxImageUnits; ++i) {
            const ImageUnitBinding& c = imageUnit_[i];
            const ImageUnitBinding& a = imageUnitApplied_[i];
            if (c.equal(a)) continue;
            sink.bindImageTexture(i, c.texture, c.level, c.layered, c.layer,
                                  c.access, c.format);
        }
        imageUnitApplied_ = imageUnit_;
        imageUnitsDirty_ = false;
        ++applied;
    }

    return applied;
}

namespace {

// Caps whose on/off state the tracker owns. Other capabilities (e.g. DITHER)
// are not tracked and are reported as unsupported pnames by the getters.
bool isTrackedCap(GLenum cap) {
    switch (cap) {
    case 0x0B10: // GL_POINT_SMOOTH
    case 0x0B20: // GL_LINE_SMOOTH
    case 0x0B24: // GL_LINE_STIPPLE
    case 0x0B41: // GL_POLYGON_SMOOTH
    case 0x0B42: // GL_POLYGON_STIPPLE
    case 0x0B44: // GL_CULL_FACE
    case 0x0B57: // GL_COLOR_MATERIAL
    case 0x0B60: // GL_FOG
    case 0x0B71: // GL_DEPTH_TEST
    case 0x0B90: // GL_STENCIL_TEST
    case 0x0BC0: // GL_ALPHA_TEST
    case 0x0BD0: // GL_DITHER
    case 0x0BE2: // GL_BLEND
    case 0x0BF1: // GL_INDEX_LOGIC_OP
    case 0x0BF2: // GL_COLOR_LOGIC_OP
    case 0x0C11: // GL_SCISSOR_TEST
    case 0x0D1D: // GL_NORMALIZE
    case 0x0D80: // GL_AUTO_NORMAL
    case 0x8037: // GL_POLYGON_OFFSET_FILL
    case 0x2A01: // GL_POLYGON_OFFSET_POINT
    case 0x2A02: // GL_POLYGON_OFFSET_LINE
    case 0x803A: // GL_RESCALE_NORMAL
    case 0x809D: // GL_MULTISAMPLE
    case 0x809E: // GL_SAMPLE_ALPHA_TO_COVERAGE
    case 0x809F: // GL_SAMPLE_ALPHA_TO_ONE
    case 0x80A0: // GL_SAMPLE_COVERAGE
    case 0x8642: // GL_PROGRAM_POINT_SIZE
    case 0x864F: // GL_DEPTH_CLAMP
    case 0x884F: // GL_TEXTURE_CUBE_MAP_SEAMLESS
    case 0x8861: // GL_POINT_SPRITE
    case 0x8C36: // GL_SAMPLE_SHADING
    case 0x8C89: // GL_RASTERIZER_DISCARD
    case 0x8DB9: // GL_FRAMEBUFFER_SRGB
    case 0x8F9D: // GL_PRIMITIVE_RESTART
    case 0x8FDE: // GL_PRIMITIVE_RESTART_FIXED_INDEX
    case 0x9242: // GL_DEBUG_OUTPUT_SYNCHRONOUS
    case 0x92E0: // GL_DEBUG_OUTPUT
    // GL_CLIP_DISTANCE0..7
    case 0x3000: case 0x3001: case 0x3002: case 0x3003:
    case 0x3004: case 0x3005: case 0x3006: case 0x3007:
        return true;
    default:
        return false;
    }
}

GLint capValue(const std::unordered_map<GLenum, bool>& caps, GLenum cap) {
    auto it = caps.find(cap);
    return (it != caps.end() && it->second) ? 1 /* GL_TRUE */ : 0 /* GL_FALSE */;
}

} // namespace

int GLStateTracker::getInteger(GLenum p, GLint* out) const {
    switch (p) {
    case 0x0BE2: case 0x0B44: case 0x0B71: case 0x0B90: case 0x0C11: // caps
    case 0x0BD0: case 0x8DB9: case 0x809E: // GL_DITHER / GL_FRAMEBUFFER_SRGB / GL_SAMPLE_ALPHA_TO_COVERAGE
        if (!isTrackedCap(p)) return 0;
        out[0] = capValue(capsCurrent_, p);
        return 1;
    case 0x0BA2: // GL_VIEWPORT
        out[0] = viewport_[0].x; out[1] = viewport_[0].y;
        out[2] = viewport_[0].width; out[3] = viewport_[0].height;
        return 4;
    case 0x0C10: // GL_SCISSOR_BOX
        out[0] = scissor_[0].x; out[1] = scissor_[0].y;
        out[2] = scissor_[0].width; out[3] = scissor_[0].height;
        return 4;
    case 0x80C9: out[0] = static_cast<GLint>(blendBuf_[0].srcRGB); return 1;   // BLEND_SRC_RGB
    case 0x80CA: out[0] = static_cast<GLint>(blendBuf_[0].dstRGB); return 1;   // BLEND_DST_RGB
    case 0x80CB: out[0] = static_cast<GLint>(blendBuf_[0].srcAlpha); return 1; // BLEND_SRC_ALPHA
    case 0x80CC: out[0] = static_cast<GLint>(blendBuf_[0].dstAlpha); return 1; // BLEND_DST_ALPHA
    case 0x8009: out[0] = static_cast<GLint>(blendBuf_[0].equationRGB); return 1;   // BLEND_EQUATION_RGB
    case 0x883D: out[0] = static_cast<GLint>(blendBuf_[0].equationAlpha); return 1; // BLEND_EQUATION_ALPHA
    case GL_DEPTH_WRITEMASK: out[0] = depth_.mask ? 1 : 0; return 1;
    case GL_DEPTH_FUNC: out[0] = static_cast<GLint>(depth_.func); return 1;
    case GL_CULL_FACE_MODE: out[0] = static_cast<GLint>(raster_.cull); return 1;
    case GL_FRONT_FACE: out[0] = static_cast<GLint>(raster_.front); return 1;
    case GL_POINT_SIZE:
        out[0] = static_cast<GLint>(rasterScalar_.pointSize); return 1;
    case GL_LINE_WIDTH:
        out[0] = static_cast<GLint>(rasterScalar_.lineWidth); return 1;
    case GL_POLYGON_OFFSET_FACTOR:
        out[0] = static_cast<GLint>(rasterScalar_.polygonOffsetFactor); return 1;
    case GL_POLYGON_OFFSET_UNITS:
        out[0] = static_cast<GLint>(rasterScalar_.polygonOffsetUnits); return 1;
    case GL_POINT_SIZE_MIN:
        out[0] = static_cast<GLint>(pointParam_.sizeMin); return 1;
    case GL_POINT_SIZE_MAX:
        out[0] = static_cast<GLint>(pointParam_.sizeMax); return 1;
    case GL_POINT_FADE_THRESHOLD_SIZE:
        out[0] = static_cast<GLint>(pointParam_.fadeThreshold); return 1;
    case GL_POINT_SPRITE_COORD_ORIGIN:
        out[0] = static_cast<GLint>(pointParam_.spriteCoordOrigin); return 1;
    case GL_CLIP_ORIGIN:
        out[0] = static_cast<GLint>(clip_.origin); return 1;
    case GL_CLIP_DEPTH_MODE:
        out[0] = static_cast<GLint>(clip_.depth); return 1;
    case GL_CURRENT_PROGRAM: out[0] = static_cast<GLint>(activeProgram_); return 1;
    case GL_ACTIVE_TEXTURE:
        out[0] = static_cast<GLint>(GL_TEXTURE0 + activeTextureUnit_); return 1;
    case GL_SAMPLER_BINDING:
        out[0] = static_cast<GLint>(
            boundSamplerForUnit(activeTextureUnit_)); return 1;
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        out[0] = static_cast<GLint>(kMaxTextureUnits); return 1;
    case GL_LOGIC_OP_MODE:
        out[0] = static_cast<GLint>(logicOp_.mode); return 1;
    case 0x8F9E: // GL_PRIMITIVE_RESTART_INDEX
        out[0] = static_cast<GLint>(primitiveRestart_.index); return 1;
    case GL_PROVOKING_VERTEX:
        out[0] = static_cast<GLint>(provokingVertex_.mode); return 1;
    case 0x891C: // GL_CLAMP_READ_COLOR
        out[0] = static_cast<GLint>(clampColor_.readColor); return 1;
    case GL_COLOR_WRITEMASK:
        out[0] = colorMask_[0].r ? 1 : 0; out[1] = colorMask_[0].g ? 1 : 0;
        out[2] = colorMask_[0].b ? 1 : 0; out[3] = colorMask_[0].a ? 1 : 0;
        return 4;
    case 0x0B40: // GL_POLYGON_MODE (front, back)
        out[0] = static_cast<GLint>(polygonMode_.front);
        out[1] = static_cast<GLint>(polygonMode_.back);
        return 2;
    case 0x8E51: // GL_SAMPLE_MASK (one value per mask word)
        for (uint32_t i = 0; i < kMaxSampleMaskWords; ++i)
            out[i] = static_cast<GLint>(multisampleRaster_.sampleMask[i]);
        return static_cast<int>(kMaxSampleMaskWords);
    case 0x0B91: // GL_STENCIL_CLEAR_VALUE
        out[0] = static_cast<GLint>(clearStencil_.stencil);
        return 1;
    // No earlier case matched: fall through to the pixel-store block below.
    }
    // Pixel store parameters (SPEC §8.4)
    {
        GLint pv = 0;
        if (getPixelStorei(p, &pv)) { out[0] = pv; return 1; }
    }
    return 0;
}

int GLStateTracker::getBoolean(GLenum p, GLboolean* out) const {
    if (isTrackedCap(p)) {
        out[0] = static_cast<GLboolean>(capValue(capsCurrent_, p));
        return 1;
    }
    if (p == 0x0B72 /* GL_DEPTH_WRITEMASK */) {
        out[0] = depth_.mask ? 1 : 0;
        return 1;
    }
    if (p == GL_COLOR_WRITEMASK) {
        out[0] = static_cast<GLboolean>(colorMask_[0].r ? 1 : 0);
        out[1] = static_cast<GLboolean>(colorMask_[0].g ? 1 : 0);
        out[2] = static_cast<GLboolean>(colorMask_[0].b ? 1 : 0);
        out[3] = static_cast<GLboolean>(colorMask_[0].a ? 1 : 0);
        return 4;
    }
    if (p == GL_SAMPLE_COVERAGE_INVERT) {
        out[0] = static_cast<GLboolean>(sampleCoverage_.invert ? 1 : 0);
        return 1;
    }
    // Pixel store parameters (SPEC §8.4): SWAP_BYTES/LSB_FIRST are booleans;
    // the remaining integer params read back as TRUE when non-zero.
    {
        GLint pv = 0;
        if (getPixelStorei(p, &pv)) {
            out[0] = static_cast<GLboolean>(pv ? 1 : 0);
            return 1;
        }
    }
    return 0;
}

int GLStateTracker::getFloat(GLenum p, GLfloat* out) const {
    switch (p) {
    case 0x0C22: // GL_COLOR_CLEAR_VALUE
        out[0] = clearColor_.r; out[1] = clearColor_.g;
        out[2] = clearColor_.b; out[3] = clearColor_.a;
        return 4;
    case 0x0B73: // GL_DEPTH_CLEAR_VALUE
        out[0] = static_cast<GLfloat>(clearDepth_.depth);
        return 1;
    case 0x0B70: // GL_DEPTH_RANGE
        out[0] = static_cast<GLfloat>(depthRange_[0].nearVal);
        out[1] = static_cast<GLfloat>(depthRange_[0].farVal);
        return 2;
    case 0x0B11: // GL_POINT_SIZE
        out[0] = rasterScalar_.pointSize; return 1;
    case 0x0B21: // GL_LINE_WIDTH
        out[0] = rasterScalar_.lineWidth; return 1;
    case 0x8038: // GL_POLYGON_OFFSET_FACTOR
        out[0] = rasterScalar_.polygonOffsetFactor; return 1;
    case 0x2A00: // GL_POLYGON_OFFSET_UNITS
        out[0] = rasterScalar_.polygonOffsetUnits; return 1;
    case GL_POINT_SIZE_MIN:
        out[0] = pointParam_.sizeMin; return 1;
    case GL_POINT_SIZE_MAX:
        out[0] = pointParam_.sizeMax; return 1;
    case GL_POINT_FADE_THRESHOLD_SIZE:
        out[0] = pointParam_.fadeThreshold; return 1;
    case GL_POINT_SPRITE_COORD_ORIGIN:
        out[0] = static_cast<GLfloat>(pointParam_.spriteCoordOrigin); return 1;
    case GL_CLIP_ORIGIN:
        out[0] = static_cast<GLfloat>(clip_.origin); return 1;
    case GL_CLIP_DEPTH_MODE:
        out[0] = static_cast<GLfloat>(clip_.depth); return 1;
    case 0x8005: // GL_BLEND_COLOR
        out[0] = blendColor_.r; out[1] = blendColor_.g;
        out[2] = blendColor_.b; out[3] = blendColor_.a;
        return 4;
    case 0x0BA2: // GL_VIEWPORT (cast int -> float)
        out[0] = static_cast<GLfloat>(viewport_[0].x);
        out[1] = static_cast<GLfloat>(viewport_[0].y);
        out[2] = static_cast<GLfloat>(viewport_[0].width);
        out[3] = static_cast<GLfloat>(viewport_[0].height);
        return 4;
    case 0x0C10: // GL_SCISSOR_BOX (cast)
        out[0] = static_cast<GLfloat>(scissor_[0].x);
        out[1] = static_cast<GLfloat>(scissor_[0].y);
        out[2] = static_cast<GLfloat>(scissor_[0].width);
        out[3] = static_cast<GLfloat>(scissor_[0].height);
        return 4;
    case GL_SAMPLE_COVERAGE_VALUE: // 0x80B9
        out[0] = sampleCoverage_.value; return 1;
    case 0x8C36: // GL_MIN_SAMPLE_SHADING
        out[0] = multisampleRaster_.minSampleShading; return 1;
    }
    // Pixel store parameters (SPEC §8.4) — returned as float.
    {
        GLint pv = 0;
        if (getPixelStorei(p, &pv)) { out[0] = static_cast<GLfloat>(pv); return 1; }
    }
    return 0;
}

int GLStateTracker::getDouble(GLenum p, GLdouble* out) const {
    switch (p) {
    case 0x0C22:
        out[0] = clearColor_.r; out[1] = clearColor_.g;
        out[2] = clearColor_.b; out[3] = clearColor_.a;
        return 4;
    case 0x0B73:
        out[0] = clearDepth_.depth;
        return 1;
    case 0x0B70:
        out[0] = depthRange_[0].nearVal;
        out[1] = depthRange_[0].farVal;
        return 2;
    case 0x0B11: // GL_POINT_SIZE
        out[0] = rasterScalar_.pointSize; return 1;
    case 0x0B21: // GL_LINE_WIDTH
        out[0] = rasterScalar_.lineWidth; return 1;
    case 0x8038: // GL_POLYGON_OFFSET_FACTOR
        out[0] = rasterScalar_.polygonOffsetFactor; return 1;
    case 0x2A00: // GL_POLYGON_OFFSET_UNITS
        out[0] = rasterScalar_.polygonOffsetUnits; return 1;
    case GL_POINT_SIZE_MIN:
        out[0] = pointParam_.sizeMin; return 1;
    case GL_POINT_SIZE_MAX:
        out[0] = pointParam_.sizeMax; return 1;
    case GL_POINT_FADE_THRESHOLD_SIZE:
        out[0] = pointParam_.fadeThreshold; return 1;
    case GL_POINT_SPRITE_COORD_ORIGIN:
        out[0] = static_cast<GLdouble>(pointParam_.spriteCoordOrigin); return 1;
    case GL_CLIP_ORIGIN:
        out[0] = static_cast<GLdouble>(clip_.origin); return 1;
    case GL_CLIP_DEPTH_MODE:
        out[0] = static_cast<GLdouble>(clip_.depth); return 1;
    case 0x8005:
        out[0] = blendColor_.r; out[1] = blendColor_.g;
        out[2] = blendColor_.b; out[3] = blendColor_.a;
        return 4;
    case 0x0BA2:
        out[0] = viewport_[0].x; out[1] = viewport_[0].y;
        out[2] = viewport_[0].width; out[3] = viewport_[0].height;
        return 4;
    case 0x0C10:
        out[0] = scissor_[0].x; out[1] = scissor_[0].y;
        out[2] = scissor_[0].width; out[3] = scissor_[0].height;
        return 4;
    }
    // Pixel store parameters (SPEC §8.4) — returned as double.
    {
        GLint pv = 0;
        if (getPixelStorei(p, &pv)) { out[0] = static_cast<GLdouble>(pv); return 1; }
    }
    return 0;
}

bool GLStateTracker::isCapabilityEnabled(GLenum cap, bool* enabled) const {
    if (!isTrackedCap(cap)) return false;
    auto it = capsCurrent_.find(cap);
    *enabled = (it != capsCurrent_.end() && it->second);
    return true;
}

void GLStateTracker::reset() {
    capsCurrent_.clear();
    capsApplied_.clear();
    capsDirty_ = false;
    // SPEC §17.3.7: dithering is enabled by default.
    capsCurrent_[0x0BD0 /* GL_DITHER */] = true;
    capsApplied_[0x0BD0 /* GL_DITHER */] = true;
    indexedCapsCurrent_.clear();
    indexedCapsApplied_.clear();
    indexedCapsDirty_ = false;
    activeProgram_ = 0;
    activeProgramApplied_ = 0;
    programDirty_ = false;
    boundProgramPipeline_ = 0;
    boundProgramPipelineApplied_ = 0;
    programPipelineDirty_ = false;
    blendBuf_.assign(kMaxDrawBuffers, BlendState{});
    blendBufApplied_.assign(kMaxDrawBuffers, BlendState{});
    blendColor_ = BlendColorState{};
    blendColorApplied_ = BlendColorState{};
    depth_ = DepthState{};
    depthApplied_ = DepthState{};
    for (uint32_t i = 0; i < kMaxViewports; ++i) {
        depthRange_[i] = DepthRangeState{};
        depthRangeApplied_[i] = DepthRangeState{};
    }
    stencilFront_ = StencilFaceState{};
    stencilBack_ = StencilFaceState{};
    stencilFrontApplied_ = StencilFaceState{};
    stencilBackApplied_ = StencilFaceState{};
    raster_ = RasterState{};
    rasterApplied_ = RasterState{};
    rasterScalar_ = RasterScalarState{};
    rasterScalarApplied_ = RasterScalarState{};
    pointParam_ = PointParamState{};
    pointParamApplied_ = PointParamState{};
    patch_ = PatchParameterState{};
    patchApplied_ = PatchParameterState{};
    clip_ = ClipControlState{};
    clipApplied_ = ClipControlState{};
    pixel_ = PixelStoreState{};
    pixelApplied_ = PixelStoreState{};
    for (uint32_t i = 0; i < kMaxViewports; ++i) {
        viewport_[i] = ViewportState{};
        viewportApplied_[i] = ViewportState{};
        scissor_[i] = ScissorBoxState{};
        scissorApplied_[i] = ScissorBoxState{};
    }
    clearColor_ = ClearColorState{};
    clearColorApplied_ = ClearColorState{};
    clearDepth_ = ClearDepthState{};
    clearDepthApplied_ = ClearDepthState{};
    clearStencil_ = ClearStencilState{};
    clearStencilApplied_ = ClearStencilState{};
    fbBuffers_ = FramebufferBufferState{};
    fbBuffersApplied_ = FramebufferBufferState{};
    logicOp_ = LogicOpState{};
    logicOpApplied_ = LogicOpState{};
    colorMask_.assign(kMaxDrawBuffers, ColorMaskState{});
    colorMaskApplied_.assign(kMaxDrawBuffers, ColorMaskState{});
    sampleCoverage_ = SampleCoverageState{};
    sampleCoverageApplied_ = SampleCoverageState{};
    polygonMode_ = PolygonModeState{};
    polygonModeApplied_ = PolygonModeState{};
    multisampleRaster_ = MultisampleRasterState{};
    multisampleRasterApplied_ = MultisampleRasterState{};
    provokingVertex_ = ProvokingVertexState{};
    provokingVertexApplied_ = ProvokingVertexState{};
    clampColor_ = ClampColorState{};
    clampColorApplied_ = ClampColorState{};
    primitiveRestart_ = PrimitiveRestartState{};
    primitiveRestartApplied_ = PrimitiveRestartState{};
    texUnits_.assign(kMaxTextureUnits, TextureUnitState{});
    texUnitsApplied_.assign(kMaxTextureUnits, TextureUnitState{});
    activeTextureUnit_ = 0;
    activeTextureApplied_ = 0;
    textureUnitsDirty_ = false;
    samplerBound_.assign(kMaxTextureUnits, 0);
    samplerBoundApplied_.assign(kMaxTextureUnits, 0);
    samplerUnitsDirty_ = false;
    imageUnit_.assign(kMaxImageUnits, ImageUnitBinding{});
    imageUnitApplied_.assign(kMaxImageUnits, ImageUnitBinding{});
    imageUnitsDirty_ = false;
}

} // namespace glcompat
