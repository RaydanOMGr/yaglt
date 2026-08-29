#include "glcompat/state/gl_state.hpp"

namespace glcompat {

GLStateTracker::GLStateTracker() {
    texUnits_.resize(kMaxTextureUnits);
    texUnitsApplied_.resize(kMaxTextureUnits);
    samplerBound_.assign(kMaxTextureUnits, 0);
    samplerBoundApplied_.assign(kMaxTextureUnits, 0);
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
    if (colorMask_.r == r && colorMask_.g == g && colorMask_.b == b &&
        colorMask_.a == a)
        return false;
    colorMask_.r = r;
    colorMask_.g = g;
    colorMask_.b = b;
    colorMask_.a = a;
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
    if (pname == 0x0CF5 /* GL_UNPACK_ALIGNMENT */ &&
        pixel_.unpackAlignment == param)
        return false;
    if (pname == 0x0CF5) pixel_.unpackAlignment = param;
    return true;
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
                rasterScalarApplied_.polygonOffsetUnits)
            sink.polygonOffset(rasterScalar_.polygonOffsetFactor,
                               rasterScalar_.polygonOffsetUnits);
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

    if (!clip_.equal(clipApplied_)) {
        sink.clipControl(clip_.origin, clip_.depth);
        clipApplied_ = clip_;
        ++applied;
    }

    if (!pixel_.equal(pixelApplied_)) {
        sink.pixelStorei(0x0CF5 /* GL_UNPACK_ALIGNMENT */,
                         pixel_.unpackAlignment);
        pixelApplied_ = pixel_;
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

    if (!colorMask_.equal(colorMaskApplied_)) {
        sink.colorMask(colorMask_.r, colorMask_.g, colorMask_.b, colorMask_.a);
        colorMaskApplied_ = colorMask_;
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

    return applied;
}

namespace {

// Caps whose on/off state the tracker owns. Other capabilities (e.g. DITHER)
// are not tracked and are reported as unsupported pnames by the getters.
bool isTrackedCap(GLenum cap) {
    return cap == 0x0BE2 /* GL_BLEND */ || cap == 0x0B44 /* GL_CULL_FACE */ ||
           cap == 0x0B71 /* GL_DEPTH_TEST */ ||
           cap == 0x0B90 /* GL_STENCIL_TEST */ ||
           cap == 0x0BD0 /* GL_DITHER */ ||
           cap == 0x0BDA /* GL_FRAMEBUFFER_SRGB */ ||
           cap == 0x809E /* GL_SAMPLE_ALPHA_TO_COVERAGE */ ||
           cap == 0x0C11 /* GL_SCISSOR_TEST */ ||
           cap == 0x8037 /* GL_POLYGON_OFFSET_FILL */ ||
            cap == 0x0BF2 /* GL_COLOR_LOGIC_OP */ ||
            cap == 0x8F9D /* GL_PRIMITIVE_RESTART */ ||
            cap == 0x8FDE /* GL_PRIMITIVE_RESTART_FIXED_INDEX */;
}

GLint capValue(const std::unordered_map<GLenum, bool>& caps, GLenum cap) {
    auto it = caps.find(cap);
    return (it != caps.end() && it->second) ? 1 /* GL_TRUE */ : 0 /* GL_FALSE */;
}

} // namespace

int GLStateTracker::getInteger(GLenum p, GLint* out) const {
    switch (p) {
    case 0x0BE2: case 0x0B44: case 0x0B71: case 0x0B90: case 0x0C11: // caps
    case 0x0BD0: case 0x0BDA: case 0x809E: // GL_DITHER / GL_FRAMEBUFFER_SRGB / GL_SAMPLE_ALPHA_TO_COVERAGE
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
        out[0] = colorMask_.r ? 1 : 0; out[1] = colorMask_.g ? 1 : 0;
        out[2] = colorMask_.b ? 1 : 0; out[3] = colorMask_.a ? 1 : 0;
        return 4;
    case 0x0B40: // GL_POLYGON_MODE (front, back)
        out[0] = static_cast<GLint>(polygonMode_.front);
        out[1] = static_cast<GLint>(polygonMode_.back);
        return 2;
    case 0x8E51: // GL_SAMPLE_MASK (one value per mask word)
        for (uint32_t i = 0; i < kMaxSampleMaskWords; ++i)
            out[i] = static_cast<GLint>(multisampleRaster_.sampleMask[i]);
        return static_cast<int>(kMaxSampleMaskWords);
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
        out[0] = static_cast<GLboolean>(colorMask_.r ? 1 : 0);
        out[1] = static_cast<GLboolean>(colorMask_.g ? 1 : 0);
        out[2] = static_cast<GLboolean>(colorMask_.b ? 1 : 0);
        out[3] = static_cast<GLboolean>(colorMask_.a ? 1 : 0);
        return 4;
    }
    if (p == GL_SAMPLE_COVERAGE_INVERT) {
        out[0] = static_cast<GLboolean>(sampleCoverage_.invert ? 1 : 0);
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
    blend_ = BlendState{};
    blendApplied_ = BlendState{};
    blendColor_ = BlendColorState{};
    blendColorApplied_ = BlendColorState{};
    depth_ = DepthState{};
    depthApplied_ = DepthState{};
    depthRange_ = DepthRangeState{};
    depthRangeApplied_ = DepthRangeState{};
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
    fbBuffers_ = FramebufferBufferState{};
    fbBuffersApplied_ = FramebufferBufferState{};
    logicOp_ = LogicOpState{};
    logicOpApplied_ = LogicOpState{};
    colorMask_ = ColorMaskState{};
    colorMaskApplied_ = ColorMaskState{};
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
}

} // namespace glcompat
