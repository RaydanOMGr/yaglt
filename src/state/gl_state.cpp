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
    if (blend_.src == sfactor && blend_.dst == dfactor) return false;
    blend_.src = sfactor;
    blend_.dst = dfactor;
    return true;
}

bool GLStateTracker::setBlendEquation(GLenum mode) {
    if (blend_.equation == mode) return false;
    blend_.equation = mode;
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
        sink.blendFunc(blend_.src, blend_.dst);
        sink.blendEquation(blend_.equation);
        blendApplied_ = blend_;
        ++applied;
    }

    if (!depth_.equal(depthApplied_)) {
        sink.depthFunc(depth_.func);
        sink.depthMask(depth_.mask);
        depthApplied_ = depth_;
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

    return applied;
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
    depth_ = DepthState{};
    depthApplied_ = DepthState{};
    stencil_ = StencilState{};
    stencilApplied_ = StencilState{};
    raster_ = RasterState{};
    rasterApplied_ = RasterState{};
    pixel_ = PixelStoreState{};
    pixelApplied_ = PixelStoreState{};
}

} // namespace glcompat
