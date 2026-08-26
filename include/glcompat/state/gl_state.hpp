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

    // --- Blend (SPEC §10 / §17.3) ---
    // glBlendFunc sets both RGB and alpha factors to the same pair;
    // glBlendFuncSeparate sets them independently.
    bool setBlendFunc(GLenum sfactor, GLenum dfactor);
    bool setBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
                             GLenum srcAlpha, GLenum dstAlpha);
    // glBlendEquation sets both RGB and alpha equations; glBlendEquationSeparate
    // sets them independently.
    bool setBlendEquation(GLenum mode);
    bool setBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
    // glBlendColor sets the constant blend color (GL_CONSTANT_* factors).
    bool setBlendColor(float r, float g, float b, float a);

    // --- Depth ---
    bool setDepthFunc(GLenum func);
    bool setDepthMask(bool enabled);
    bool setDepthRange(double nearVal, double farVal);

    // --- Stencil ---
    bool setStencilFunc(GLenum func, GLint ref, GLuint mask);
    bool setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
    bool setStencilMask(GLuint mask);

    // --- Rasterization ---
    bool setCullFace(GLenum mode);
    bool setFrontFace(GLenum mode);

    // --- Pixel store ---
    bool setPixelStorei(GLenum pname, GLint param);

    // --- Viewport (glViewport) ---
    bool setViewport(GLint x, GLint y, GLsizei width, GLsizei height);

    // --- Scissor box (glScissor); the scissor test is GL_SCISSOR_TEST cap ---
    bool setScissor(GLint x, GLint y, GLsizei width, GLsizei height);

    // --- Clear values (glClearColor / glClearDepth, SPEC §2.1) ---
    bool setClearColor(float r, float g, float b, float a);
    bool setClearDepth(double d);

    // Push only changed state to `sink`. Returns number of categories applied.
    int apply(GLStateSink& sink);

    // Reset both current and applied state (e.g. on context (re)init).
    void reset();

private:
    struct BlendState {
        GLenum srcRGB = 1;   // GL_ONE
        GLenum dstRGB = 0;   // GL_ZERO
        GLenum srcAlpha = 1; // GL_ONE
        GLenum dstAlpha = 0; // GL_ZERO
        GLenum equationRGB = 0x8006;   // GL_FUNC_ADD
        GLenum equationAlpha = 0x8006; // GL_FUNC_ADD
        bool equal(const BlendState& o) const {
            return srcRGB == o.srcRGB && dstRGB == o.dstRGB &&
                   srcAlpha == o.srcAlpha && dstAlpha == o.dstAlpha &&
                   equationRGB == o.equationRGB && equationAlpha == o.equationAlpha;
        }
    };
    struct BlendColorState {
        float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
        bool equal(const BlendColorState& o) const {
            return r == o.r && g == o.g && b == o.b && a == o.a;
        }
    };
    struct DepthState {
        GLenum func = 0x0201; // GL_LESS
        bool mask = true;
        bool equal(const DepthState& o) const {
            return func == o.func && mask == o.mask;
        }
    };
    struct DepthRangeState {
        double nearVal = 0.0;
        double farVal = 1.0;
        bool equal(const DepthRangeState& o) const {
            return nearVal == o.nearVal && farVal == o.farVal;
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
    struct ViewportState {
        GLint x = 0;
        GLint y = 0;
        GLsizei width = 0;
        GLsizei height = 0;
        bool equal(const ViewportState& o) const {
            return x == o.x && y == o.y && width == o.width &&
                   height == o.height;
        }
    };
    struct ScissorBoxState {
        GLint x = 0;
        GLint y = 0;
        GLsizei width = 0;
        GLsizei height = 0;
        bool equal(const ScissorBoxState& o) const {
            return x == o.x && y == o.y && width == o.width &&
                   height == o.height;
        }
    };
    struct ClearColorState {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;
        bool equal(const ClearColorState& o) const {
            return r == o.r && g == o.g && b == o.b && a == o.a;
        }
    };
    struct ClearDepthState {
        double depth = 1.0;
        bool equal(const ClearDepthState& o) const {
            return depth == o.depth;
        }
    };

    std::unordered_map<GLenum, bool> capsCurrent_;
    std::unordered_map<GLenum, bool> capsApplied_;
    bool capsDirty_ = false;

    GLObjectName activeProgram_ = 0;
    GLObjectName activeProgramApplied_ = 0;
    bool programDirty_ = false;

    BlendState blend_, blendApplied_;
    BlendColorState blendColor_, blendColorApplied_;
    DepthState depth_, depthApplied_;
    DepthRangeState depthRange_, depthRangeApplied_;
    StencilState stencil_, stencilApplied_;
    RasterState raster_, rasterApplied_;
    PixelStoreState pixel_, pixelApplied_;
    ViewportState viewport_, viewportApplied_;
    ScissorBoxState scissor_, scissorApplied_;
    ClearColorState clearColor_, clearColorApplied_;
    ClearDepthState clearDepth_, clearDepthApplied_;
};

} // namespace glcompat
