#include "glcompat/state/gl_state.hpp"

namespace glcompat {

GLStateTracker::GLStateTracker() = default;

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

bool GLStateTracker::useProgram(GLObjectName prog) {
    if (activeProgram_ == prog) return false;
    activeProgram_ = prog;
    programDirty_ = true;
    return true;
}

bool GLStateTracker::setBlendFunc(GLenum sfactor, GLenum dfactor) {
    if (blend_.srcRGB == sfactor && blend_.dstRGB == dfactor &&
        blend_.srcAlpha == sfactor && blend_.dstAlpha == dfactor)
        return false;
    blend_.srcRGB = blend_.srcAlpha = sfactor;
    blend_.dstRGB = blend_.dstAlpha = dfactor;
    return true;
}

bool GLStateTracker::setBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
                                          GLenum srcAlpha, GLenum dstAlpha) {
    if (blend_.srcRGB == srcRGB && blend_.dstRGB == dstRGB &&
        blend_.srcAlpha == srcAlpha && blend_.dstAlpha == dstAlpha)
        return false;
    blend_.srcRGB = srcRGB;
    blend_.dstRGB = dstRGB;
    blend_.srcAlpha = srcAlpha;
    blend_.dstAlpha = dstAlpha;
    return true;
}

bool GLStateTracker::setBlendEquation(GLenum mode) {
    if (blend_.equationRGB == mode && blend_.equationAlpha == mode) return false;
    blend_.equationRGB = blend_.equationAlpha = mode;
    return true;
}

bool GLStateTracker::setBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    if (blend_.equationRGB == modeRGB && blend_.equationAlpha == modeAlpha)
        return false;
    blend_.equationRGB = modeRGB;
    blend_.equationAlpha = modeAlpha;
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
    if (depthRange_.nearVal == nearVal && depthRange_.farVal == farVal)
        return false;
    depthRange_.nearVal = nearVal;
    depthRange_.farVal = farVal;
    return true;
}

bool GLStateTracker::setStencilFunc(GLenum func, GLint ref, GLuint mask) {
    if (stencil_.func == func && stencil_.ref == ref && stencil_.mask == mask)
        return false;
    stencil_.func = func;
    stencil_.ref = ref;
    stencil_.mask = mask;
    return true;
}

bool GLStateTracker::setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) {
    if (stencil_.sfail == sfail && stencil_.dpfail == dpfail &&
        stencil_.dppass == dppass)
        return false;
    stencil_.sfail = sfail;
    stencil_.dpfail = dpfail;
    stencil_.dppass = dppass;
    return true;
}

bool GLStateTracker::setStencilMask(GLuint mask) {
    if (stencil_.writeMask == mask) return false;
    stencil_.writeMask = mask;
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

bool GLStateTracker::setPixelStorei(GLenum pname, GLint param) {
    if (pname == 0x0CF5 /* GL_UNPACK_ALIGNMENT */ &&
        pixel_.unpackAlignment == param)
        return false;
    if (pname == 0x0CF5) pixel_.unpackAlignment = param;
    return true;
}

bool GLStateTracker::setViewport(GLint x, GLint y, GLsizei width,
                                  GLsizei height) {
    if (viewport_.x == x && viewport_.y == y && viewport_.width == width &&
        viewport_.height == height)
        return false;
    viewport_.x = x;
    viewport_.y = y;
    viewport_.width = width;
    viewport_.height = height;
    return true;
}

bool GLStateTracker::setScissor(GLint x, GLint y, GLsizei width,
                                 GLsizei height) {
    if (scissor_.x == x && scissor_.y == y && scissor_.width == width &&
        scissor_.height == height)
        return false;
    scissor_.x = x;
    scissor_.y = y;
    scissor_.width = width;
    scissor_.height = height;
    return true;
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

    if (programDirty_) {
        sink.useProgram(activeProgram_);
        activeProgramApplied_ = activeProgram_;
        programDirty_ = false;
        ++applied;
    }

    if (!blend_.equal(blendApplied_)) {
        sink.blendFuncSeparate(blend_.srcRGB, blend_.dstRGB,
                               blend_.srcAlpha, blend_.dstAlpha);
        sink.blendEquationSeparate(blend_.equationRGB, blend_.equationAlpha);
        blendApplied_ = blend_;
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

    if (!depthRange_.equal(depthRangeApplied_)) {
        sink.depthRange(depthRange_.nearVal, depthRange_.farVal);
        depthRangeApplied_ = depthRange_;
        ++applied;
    }

    if (!stencil_.equal(stencilApplied_)) {
        sink.stencilFunc(stencil_.func, stencil_.ref, stencil_.mask);
        sink.stencilOp(stencil_.sfail, stencil_.dpfail, stencil_.dppass);
        sink.stencilMask(stencil_.writeMask);
        stencilApplied_ = stencil_;
        ++applied;
    }

    if (!raster_.equal(rasterApplied_)) {
        sink.cullFace(raster_.cull);
        sink.frontFace(raster_.front);
        rasterApplied_ = raster_;
        ++applied;
    }

    if (!pixel_.equal(pixelApplied_)) {
        sink.pixelStorei(0x0CF5 /* GL_UNPACK_ALIGNMENT */,
                         pixel_.unpackAlignment);
        pixelApplied_ = pixel_;
        ++applied;
    }

    if (!viewport_.equal(viewportApplied_)) {
        sink.setViewport(viewport_.x, viewport_.y, viewport_.width,
                         viewport_.height);
        viewportApplied_ = viewport_;
        ++applied;
    }

    if (!scissor_.equal(scissorApplied_)) {
        sink.setScissor(scissor_.x, scissor_.y, scissor_.width,
                        scissor_.height);
        scissorApplied_ = scissor_;
        ++applied;
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

    return applied;
}

namespace {

// Caps whose on/off state the tracker owns. Other capabilities (e.g. DITHER)
// are not tracked and are reported as unsupported pnames by the getters.
bool isTrackedCap(GLenum cap) {
    return cap == 0x0BE2 /* GL_BLEND */ || cap == 0x0B44 /* GL_CULL_FACE */ ||
           cap == 0x0B71 /* GL_DEPTH_TEST */ ||
           cap == 0x0B90 /* GL_STENCIL_TEST */ ||
           cap == 0x0C11 /* GL_SCISSOR_TEST */;
}

GLint capValue(const std::unordered_map<GLenum, bool>& caps, GLenum cap) {
    auto it = caps.find(cap);
    return (it != caps.end() && it->second) ? 1 /* GL_TRUE */ : 0 /* GL_FALSE */;
}

} // namespace

int GLStateTracker::getInteger(GLenum p, GLint* out) const {
    switch (p) {
    case 0x0BE2: case 0x0B44: case 0x0B71: case 0x0B90: case 0x0C11: // caps
        if (!isTrackedCap(p)) return 0;
        out[0] = capValue(capsCurrent_, p);
        return 1;
    case 0x0BA2: // GL_VIEWPORT
        out[0] = viewport_.x; out[1] = viewport_.y;
        out[2] = viewport_.width; out[3] = viewport_.height;
        return 4;
    case 0x0C10: // GL_SCISSOR_BOX
        out[0] = scissor_.x; out[1] = scissor_.y;
        out[2] = scissor_.width; out[3] = scissor_.height;
        return 4;
    case 0x80C9: out[0] = static_cast<GLint>(blend_.srcRGB); return 1;   // BLEND_SRC_RGB
    case 0x80CA: out[0] = static_cast<GLint>(blend_.dstRGB); return 1;   // BLEND_DST_RGB
    case 0x80CB: out[0] = static_cast<GLint>(blend_.srcAlpha); return 1; // BLEND_SRC_ALPHA
    case 0x80CC: out[0] = static_cast<GLint>(blend_.dstAlpha); return 1; // BLEND_DST_ALPHA
    case 0x8009: out[0] = static_cast<GLint>(blend_.equationRGB); return 1;   // BLEND_EQUATION_RGB
    case 0x883D: out[0] = static_cast<GLint>(blend_.equationAlpha); return 1; // BLEND_EQUATION_ALPHA
    case GL_DEPTH_WRITEMASK: out[0] = depth_.mask ? 1 : 0; return 1;
    case GL_DEPTH_FUNC: out[0] = static_cast<GLint>(depth_.func); return 1;
    case GL_CULL_FACE_MODE: out[0] = static_cast<GLint>(raster_.cull); return 1;
    case GL_FRONT_FACE: out[0] = static_cast<GLint>(raster_.front); return 1;
    case GL_CURRENT_PROGRAM: out[0] = static_cast<GLint>(activeProgram_); return 1;
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
        out[0] = static_cast<GLfloat>(depthRange_.nearVal);
        out[1] = static_cast<GLfloat>(depthRange_.farVal);
        return 2;
    case 0x8005: // GL_BLEND_COLOR
        out[0] = blendColor_.r; out[1] = blendColor_.g;
        out[2] = blendColor_.b; out[3] = blendColor_.a;
        return 4;
    case 0x0BA2: // GL_VIEWPORT (cast int -> float)
        out[0] = static_cast<GLfloat>(viewport_.x);
        out[1] = static_cast<GLfloat>(viewport_.y);
        out[2] = static_cast<GLfloat>(viewport_.width);
        out[3] = static_cast<GLfloat>(viewport_.height);
        return 4;
    case 0x0C10: // GL_SCISSOR_BOX (cast)
        out[0] = static_cast<GLfloat>(scissor_.x);
        out[1] = static_cast<GLfloat>(scissor_.y);
        out[2] = static_cast<GLfloat>(scissor_.width);
        out[3] = static_cast<GLfloat>(scissor_.height);
        return 4;
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
        out[0] = depthRange_.nearVal;
        out[1] = depthRange_.farVal;
        return 2;
    case 0x8005:
        out[0] = blendColor_.r; out[1] = blendColor_.g;
        out[2] = blendColor_.b; out[3] = blendColor_.a;
        return 4;
    case 0x0BA2:
        out[0] = viewport_.x; out[1] = viewport_.y;
        out[2] = viewport_.width; out[3] = viewport_.height;
        return 4;
    case 0x0C10:
        out[0] = scissor_.x; out[1] = scissor_.y;
        out[2] = scissor_.width; out[3] = scissor_.height;
        return 4;
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
    activeProgram_ = 0;
    activeProgramApplied_ = 0;
    programDirty_ = false;
    blend_ = BlendState{};
    blendApplied_ = BlendState{};
    blendColor_ = BlendColorState{};
    blendColorApplied_ = BlendColorState{};
    depth_ = DepthState{};
    depthApplied_ = DepthState{};
    depthRange_ = DepthRangeState{};
    depthRangeApplied_ = DepthRangeState{};
    stencil_ = StencilState{};
    stencilApplied_ = StencilState{};
    raster_ = RasterState{};
    rasterApplied_ = RasterState{};
    pixel_ = PixelStoreState{};
    pixelApplied_ = PixelStoreState{};
    viewport_ = ViewportState{};
    viewportApplied_ = ViewportState{};
    scissor_ = ScissorBoxState{};
    scissorApplied_ = ScissorBoxState{};
    clearColor_ = ClearColorState{};
    clearColorApplied_ = ClearColorState{};
    clearDepth_ = ClearDepthState{};
    clearDepthApplied_ = ClearDepthState{};
}

} // namespace glcompat
