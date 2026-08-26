#include "glcompat/state/gl_state.hpp"

namespace glcompat {

GLStateTracker::GLStateTracker() {
    texUnits_.resize(kMaxTextureUnits);
    texUnitsApplied_.resize(kMaxTextureUnits);
    samplerBound_.assign(kMaxTextureUnits, 0);
    samplerBoundApplied_.assign(kMaxTextureUnits, 0);
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

GLObjectName GLStateTracker::boundTextureForTarget(GLenum target) const {
    if (activeTextureUnit_ >= texUnits_.size()) return 0;
    const auto& bound = texUnits_[activeTextureUnit_].bound;
    auto it = bound.find(target);
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

bool GLStateTracker::setTextureBindings(uint32_t first, uint32_t count,
                                        GLenum target,
                                        const GLObjectName* names) {
    if (first > kMaxTextureUnits || first + count > kMaxTextureUnits)
        return false; // out of range
    bool changed = false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t unit = first + i;
        GLObjectName name = (names != nullptr) ? names[i] : 0;
        auto& bound = texUnits_[unit].bound;
        auto it = bound.find(target);
        if (it == bound.end() || it->second != name) {
            if (name == 0) {
                if (!bound.empty()) { bound.clear(); changed = true; }
            } else {
                bound[target] = name;
                changed = true;
            }
        }
    }
    if (changed) textureUnitsDirty_ = true;
    return changed;
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

    if (!rasterScalar_.equal(rasterScalarApplied_)) {
        if (rasterScalar_.pointSize != rasterScalarApplied_.pointSize)
            sink.pointSize(rasterScalar_.pointSize);
        if (rasterScalar_.lineWidth != rasterScalarApplied_.lineWidth)
            sink.lineWidth(rasterScalar_.lineWidth);
        if (rasterScalar_.polygonOffsetFactor !=
                rasterScalarApplied_.polygonOffsetFactor ||
            rasterScalar_.polygonOffsetUnits !=
                rasterScalarApplied_.polygonOffsetUnits)
            sink.polygonOffset(rasterScalar_.polygonOffsetFactor,
                               rasterScalar_.polygonOffsetUnits);
        rasterScalarApplied_ = rasterScalar_;
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

    if (!fbBuffers_.equal(fbBuffersApplied_)) {
        if (!fbBuffers_.draw.empty()) {
            sink.drawBuffers(static_cast<int32_t>(fbBuffers_.draw.size()),
                             fbBuffers_.draw.data());
        }
        sink.readBuffer(fbBuffers_.read);
        fbBuffersApplied_ = fbBuffers_;
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

    return applied;
}

namespace {

// Caps whose on/off state the tracker owns. Other capabilities (e.g. DITHER)
// are not tracked and are reported as unsupported pnames by the getters.
bool isTrackedCap(GLenum cap) {
    return cap == 0x0BE2 /* GL_BLEND */ || cap == 0x0B44 /* GL_CULL_FACE */ ||
           cap == 0x0B71 /* GL_DEPTH_TEST */ ||
           cap == 0x0B90 /* GL_STENCIL_TEST */ ||
           cap == 0x0C11 /* GL_SCISSOR_TEST */ ||
           cap == 0x8037 /* GL_POLYGON_OFFSET_FILL */;
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
    case GL_POINT_SIZE:
        out[0] = static_cast<GLint>(rasterScalar_.pointSize); return 1;
    case GL_LINE_WIDTH:
        out[0] = static_cast<GLint>(rasterScalar_.lineWidth); return 1;
    case GL_POLYGON_OFFSET_FACTOR:
        out[0] = static_cast<GLint>(rasterScalar_.polygonOffsetFactor); return 1;
    case GL_POLYGON_OFFSET_UNITS:
        out[0] = static_cast<GLint>(rasterScalar_.polygonOffsetUnits); return 1;
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
    case 0x0B11: // GL_POINT_SIZE
        out[0] = rasterScalar_.pointSize; return 1;
    case 0x0B21: // GL_LINE_WIDTH
        out[0] = rasterScalar_.lineWidth; return 1;
    case 0x8038: // GL_POLYGON_OFFSET_FACTOR
        out[0] = rasterScalar_.polygonOffsetFactor; return 1;
    case 0x2A00: // GL_POLYGON_OFFSET_UNITS
        out[0] = rasterScalar_.polygonOffsetUnits; return 1;
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
    case 0x0B11: // GL_POINT_SIZE
        out[0] = rasterScalar_.pointSize; return 1;
    case 0x0B21: // GL_LINE_WIDTH
        out[0] = rasterScalar_.lineWidth; return 1;
    case 0x8038: // GL_POLYGON_OFFSET_FACTOR
        out[0] = rasterScalar_.polygonOffsetFactor; return 1;
    case 0x2A00: // GL_POLYGON_OFFSET_UNITS
        out[0] = rasterScalar_.polygonOffsetUnits; return 1;
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
    rasterScalar_ = RasterScalarState{};
    rasterScalarApplied_ = RasterScalarState{};
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
    fbBuffers_ = FramebufferBufferState{};
    fbBuffersApplied_ = FramebufferBufferState{};
    texUnits_.assign(kMaxTextureUnits, TextureUnitState{});
    texUnitsApplied_.assign(kMaxTextureUnits, TextureUnitState{});
    activeTextureUnit_ = 0;
    activeTextureApplied_ = 0;
    textureUnitsDirty_ = false;
    samplerBound_.assign(kMaxTextureUnits, 0);
    samplerBoundApplied_.assign(kMaxTextureUnits, 0);
    samplerUnitsDirty_ = false;
}

} // namespace glcompat
