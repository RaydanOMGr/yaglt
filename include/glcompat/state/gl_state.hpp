#pragma once

#include "glcompat/frontend/gl_types.hpp"
#include "glcompat/frontend/objects.hpp"
#include "glcompat/state/gl_state_sink.hpp"

#include <cstdint>
#include <unordered_map>

namespace glcompat {

// Centralized OpenGL pipeline state with change tracking.
//
// `set*` methods update the current state and return true only when the value
// actually changed. `apply(sink)` pushes to the backend only the categories
// whose current state differs from the last applied state, avoiding redundant
// backend calls (SPEC §10). State is backend-agnostic; the tracker never talks
// to a native API directly.
class GLStateTracker {
public:
    GLStateTracker();

    // --- Capabilities (glEnable / glDisable) ---
    bool setCapability(GLenum cap, bool enabled);
    bool isCapabilityEnabled(GLenum cap) const;

    // --- Active program (glUseProgram); 0 = none bound ---
    bool useProgram(GLObjectName prog);
    GLObjectName activeProgram() const { return activeProgram_; }

    // --- Blend ---
    bool setBlendFunc(GLenum sfactor, GLenum dfactor);
    bool setBlendEquation(GLenum mode);

    // --- Depth ---
    bool setDepthFunc(GLenum func);
    bool setDepthMask(bool enabled);

    // --- Stencil ---
    bool setStencilFunc(GLenum func, GLint ref, GLuint mask);
    bool setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
    bool setStencilMask(GLuint mask);

    // --- Rasterization ---
    bool setCullFace(GLenum mode);
    bool setFrontFace(GLenum mode);

    // --- Pixel store ---
    bool setPixelStorei(GLenum pname, GLint param);

    // Push only changed state to `sink`. Returns number of categories applied.
    int apply(GLStateSink& sink);

    // Reset both current and applied state (e.g. on context (re)init).
    void reset();

private:
    struct BlendState {
        GLenum src = 1;       // GL_ONE
        GLenum dst = 0;       // GL_ZERO
        GLenum equation = 0x8006; // GL_FUNC_ADD
        bool equal(const BlendState& o) const {
            return src == o.src && dst == o.dst && equation == o.equation;
        }
    };
    struct DepthState {
        GLenum func = 0x0201; // GL_LESS
        bool mask = true;
        bool equal(const DepthState& o) const {
            return func == o.func && mask == o.mask;
        }
    };
    struct StencilState {
        GLenum func = 0x0207;     // GL_ALWAYS
        GLint ref = 0;
        GLuint mask = ~0u;
        GLenum sfail = 0x1E00;    // GL_KEEP
        GLenum dpfail = 0x1E00;   // GL_KEEP
        GLenum dppass = 0x1E00;   // GL_KEEP
        GLuint writeMask = ~0u;
        bool equal(const StencilState& o) const {
            return func == o.func && ref == o.ref && mask == o.mask &&
                   sfail == o.sfail && dpfail == o.dpfail &&
                   dppass == o.dppass && writeMask == o.writeMask;
        }
    };
    struct RasterState {
        GLenum cull = 0x0405;  // GL_BACK
        GLenum front = 0x0901; // GL_CCW
        bool equal(const RasterState& o) const {
            return cull == o.cull && front == o.front;
        }
    };
    struct PixelStoreState {
        GLint unpackAlignment = 4;
        bool equal(const PixelStoreState& o) const {
            return unpackAlignment == o.unpackAlignment;
        }
    };

    std::unordered_map<GLenum, bool> capsCurrent_;
    std::unordered_map<GLenum, bool> capsApplied_;
    bool capsDirty_ = false;

    GLObjectName activeProgram_ = 0;
    GLObjectName activeProgramApplied_ = 0;
    bool programDirty_ = false;

    BlendState blend_, blendApplied_;
    DepthState depth_, depthApplied_;
    StencilState stencil_, stencilApplied_;
    RasterState raster_, rasterApplied_;
    PixelStoreState pixel_, pixelApplied_;
};

} // namespace glcompat
