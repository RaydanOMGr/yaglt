#pragma once

#include "glcompat/frontend/gl_types.hpp"
#include "glcompat/frontend/objects.hpp"
#include "glcompat/state/gl_state_sink.hpp"

#include <array>
#include <cstdint>
#include <unordered_map>

namespace glcompat {

// Maps a per-face cube-map target (GL_TEXTURE_CUBE_MAP_POSITIVE_X, ...) to its
// canonical cube-map target (GL_TEXTURE_CUBE_MAP) so that a texture bound as a
// cube map can be addressed by any of its six faces (SPEC §8.1, glTexImage2D on
// a cube map). All other targets are returned unchanged.
GLenum normalizeTextureTarget(GLenum target);

// Centralized OpenGL pipeline state with change tracking.
//
// `set*` methods update the current state and return true only when the value
// actually changed. `apply(sink)` pushes to the backend only the categories
// whose current state differs from the last applied state, avoiding redundant
// backend calls (SPEC §10). State is backend-agnostic; the tracker never talks
// to a native API directly.
class GLStateTracker {
public:
    // Maximum number of texture image units (combined). A desktop GL context
    // guarantees at least this many; GLES guarantees far fewer but YAGLT tracks
    // a fixed, generous table so unit indices stay stable (SPEC §2.1).
    static constexpr uint32_t kMaxTextureUnits = 32;
    // GL 4.6 core defines MAX_SAMPLE_MASK_WORDS = 2 (64 sample bits). The
    // frontend tracks this many mask words for glSampleMaski (SPEC §11.5).
    static constexpr uint32_t kMaxSampleMaskWords = 2;

    GLStateTracker();

    // --- Capabilities (glEnable / glDisable) ---
    bool setCapability(GLenum cap, bool enabled);
    bool isCapabilityEnabled(GLenum cap) const;

    // --- Active program (glUseProgram); 0 = none bound ---
    bool useProgram(GLObjectName prog);
    GLObjectName activeProgram() const { return activeProgram_; }

    // --- Program pipeline (glBindProgramPipeline, SPEC §7.4); 0 = none bound ---
    // Tracked independently of the single active program: when both are bound the
    // single program takes precedence, but the pipeline object is still state and
    // is forwarded to the backend via the GLStateSink.
    bool bindProgramPipeline(GLObjectName pipeline);
    GLObjectName boundProgramPipeline() const { return boundProgramPipeline_; }

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

    // --- Color write mask (SPEC §17.3.6, glColorMask) ---
    bool setColorMask(bool r, bool g, bool b, bool a);

    // --- Sample coverage (SPEC §17.3.6 multisample, glSampleCoverage) ---
    bool setSampleCoverage(float value, bool invert);

    // --- Primitive restart index (SPEC §10.4, glPrimitiveRestartIndex) ---
    // The matching GL_PRIMITIVE_RESTART capability is a normal enable/disable cap;
    // this records only the restart index, pushed to the backend on change.
    bool setPrimitiveRestartIndex(uint32_t index);
    uint32_t primitiveRestartIndex() const { return primitiveRestart_.index; }

    // --- Rasterization ---
    bool setCullFace(GLenum mode);
    bool setFrontFace(GLenum mode);

    // --- Rasterization scalar state (SPEC §11) ---
    // glPointSize / glLineWidth / glPolygonOffset. These are independent scalar
    // values pushed to the backend only when the relevant one changed.
    bool setPointSize(float size);
    bool setLineWidth(float width);
    bool setPolygonOffset(float factor, float units);

    // --- Rasterization polygon mode (SPEC §11.1, glPolygonMode) ---
    // `face` selects which side(s) the mode applies to (GL_FRONT, GL_BACK,
    // GL_FRONT_AND_BACK); `mode` is GL_POINT / GL_LINE / GL_FILL. Returns true
    // when any tracked mode changed. Invalid face/mode are rejected by the
    // caller (GL_INVALID_ENUM) and leave state untouched.
    bool setPolygonMode(GLenum face, GLenum mode);

    // --- Multisample raster state (SPEC §11.5) ---
    // glSampleMaski sets one mask word; `maskNumber` must be < kMaxSampleMaskWords
    // (the caller reports GL_INVALID_VALUE otherwise). glMinSampleShading selects
    // the minimum sample-shading fraction in [0,1].
    bool setSampleMaski(GLuint maskNumber, GLuint mask);
    bool setMinSampleShading(float value);

    // --- Provoking vertex (SPEC §11, glProvokingVertex) ---
    // `mode` is GL_FIRST_VERTEX_CONVENTION / GL_LAST_VERTEX_CONVENTION (default
    // LAST). Pushed to the backend only when the mode changes (SPEC §10). The
    // caller rejects an invalid mode with GL_INVALID_ENUM.
    bool setProvokingVertex(GLenum mode);
    GLenum provokingVertex() const { return provokingVertex_.mode; }

    // --- Pixel store ---
    bool setPixelStorei(GLenum pname, GLint param);

    // --- Viewport (glViewport) ---
    bool setViewport(GLint x, GLint y, GLsizei width, GLsizei height);

    // --- Scissor box (glScissor); the scissor test is GL_SCISSOR_TEST cap ---
    bool setScissor(GLint x, GLint y, GLsizei width, GLsizei height);

    // --- Clear values (glClearColor / glClearDepth, SPEC §2.1) ---
    bool setClearColor(float r, float g, float b, float a);
    bool setClearDepth(double d);

    // --- Color logic op (SPEC §17.3.4, glLogicOp) ---
    // The logic op mode is pushed to the backend only when it changes; it is
    // applied by the driver only while GL_COLOR_LOGIC_OP is enabled.
    bool setLogicOp(GLenum mode);

    // --- Whole-framebuffer buffer selection (SPEC §15 / §16) ---
    // `drawBuffers` selects the draw buffers for the bound framebuffer (vector of
    // GL_COLOR_ATTACHMENTi / GL_BACK / GL_NONE); `readBuffer` selects its read
    // buffer. Both return true when the selection changed.
    bool setDrawBuffers(const std::vector<GLenum>& bufs);
    bool setReadBuffer(GLenum buf);

    // --- Color clamping (SPEC §15.2.3, glClampColor) ---
    // `target` must be GL_CLAMP_READ_COLOR (the only core target; the caller
    // reports GL_INVALID_ENUM otherwise); `mode` is GL_TRUE / GL_FALSE /
    // GL_FIXED_ONLY (default GL_FIXED_ONLY). Pushed only when the mode changes.
    bool setClampColor(GLenum target, GLenum mode);
    GLenum clampReadColor() const { return clampColor_.readColor; }

    // --- Texture units (SPEC §2.1) ---
    // glActiveTexture selects the unit (texture = GL_TEXTURE0 + i); returns
    // true when the active unit actually changed. glBindTexture binds `name` to
    // `target` on the active unit; returns true when that (unit,target) binding
    // changed. boundTextureForTarget returns the texture bound to `target` on
    // the active unit (0 when none is bound).
    bool setActiveTexture(GLenum texture);
    uint32_t activeTextureUnit() const { return activeTextureUnit_; }
    bool setTextureBinding(GLenum target, GLObjectName name);
    GLObjectName boundTextureForTarget(GLenum target) const;

    // Direct State Access texture binding (SPEC §2.1, `glBindTextureUnit`).
    // Binds `name` to `target` on a specific `unit` (zero-based, not the active
    // unit) without touching the active-texture selector. Returns true when that
    // (unit, target) binding changed. An out-of-range `unit` returns false (the
    // caller reports GL_INVALID_VALUE); binding 0 clears every target on the
    // unit (unbind).
    bool setTextureUnitBinding(uint32_t unit, GLenum target, GLObjectName name);
    // Binds an array of textures to consecutive units [first, first+count) for
    // `target` (`glBindTextures`). `names` may be null (treated as all-zero).
    // Returns true when any (unit, target) binding changed. Out-of-range bounds
    // return false (caller reports GL_INVALID_VALUE).
    bool setTextureBindings(uint32_t first, uint32_t count, GLenum target,
                            const GLObjectName* names);
    // Texture bound to `target` on a specific `unit` (0 when none).
    GLObjectName boundTextureForUnitTarget(uint32_t unit, GLenum target) const;

    // Clears any (unit, target) binding that references `name` (used when a
    // texture object is deleted). Returns true when a binding was changed.
    bool clearTextureBinding(GLObjectName name);
    uint32_t maxCombinedTextureUnits() const { return kMaxTextureUnits; }

    // --- Sampler objects (SPEC §8.2) ---
    // Bind sampler `name` to texture unit `unit`; returns true when the binding
    // changed. boundSamplerForUnit returns the sampler bound to `unit` (0 when
    // none). clearSamplerBinding resets any unit bound to `name` (used when the
    // sampler object is deleted), returning true when a binding changed.
    bool setSamplerBinding(uint32_t unit, GLObjectName name);
    GLObjectName boundSamplerForUnit(uint32_t unit) const;
    bool clearSamplerBinding(GLObjectName name);

    // Push only changed state to `sink`. Returns number of categories applied.
    int apply(GLStateSink& sink);

    // Reset both current and applied state (e.g. on context (re)init).
    void reset();

    // --- State queries (glGet*, glIsEnabled; SPEC §22) ---
    // Return the number of values written for `pname`, or 0 when the pname is
    // not tracked here (the caller then reports GL_INVALID_ENUM). The frontend
    // owns these values so glGet never has to query the backend driver (SPEC
    // §10: avoid redundant backend calls).
    int getInteger(GLenum pname, GLint* out) const;
    int getBoolean(GLenum pname, GLboolean* out) const;
    int getFloat(GLenum pname, GLfloat* out) const;
    int getDouble(GLenum pname, GLdouble* out) const;
    // Returns true when `cap` is a tracked capability; the result is written to
    // `*enabled`. For an untracked cap the return is false (caller reports
    // GL_INVALID_ENUM, mirroring desktop GL).
    bool isCapabilityEnabled(GLenum cap, bool* enabled) const;

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
    struct RasterScalarState {
        float pointSize = 1.0f;
        float lineWidth = 1.0f;
        float polygonOffsetFactor = 0.0f;
        float polygonOffsetUnits = 0.0f;
        bool equal(const RasterScalarState& o) const {
            return pointSize == o.pointSize && lineWidth == o.lineWidth &&
                   polygonOffsetFactor == o.polygonOffsetFactor &&
                   polygonOffsetUnits == o.polygonOffsetUnits;
        }
    };
    // glPolygonMode (SPEC §11.1). Per-side render mode (GL_POINT/GL_LINE/GL_FILL).
    struct PolygonModeState {
        GLenum front = 0x1B02; // GL_FILL
        GLenum back = 0x1B02;  // GL_FILL
        bool equal(const PolygonModeState& o) const {
            return front == o.front && back == o.back;
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
    struct FramebufferBufferState {
        std::vector<GLenum> draw;            // draw buffer selection
        GLenum read = 0x0405;                // GL_BACK (default read buffer)
        bool equal(const FramebufferBufferState& o) const {
            return draw == o.draw && read == o.read;
        }
    };
    struct LogicOpState {
        GLenum mode = 0x1503; // GL_COPY (default logic op)
        bool equal(const LogicOpState& o) const { return mode == o.mode; }
    };
    struct ColorMaskState {
        bool r = true, g = true, b = true, a = true;
        bool equal(const ColorMaskState& o) const {
            return r == o.r && g == o.g && b == o.b && a == o.a;
        }
    };
    struct SampleCoverageState {
        float value = 1.0f;
        bool invert = false;
        bool equal(const SampleCoverageState& o) const {
            return value == o.value && invert == o.invert;
        }
    };
    struct PrimitiveRestartState {
        uint32_t index = 0;
        bool equal(const PrimitiveRestartState& o) const {
            return index == o.index;
        }
    };
    // Multisample raster state (SPEC §11.5). Sample mask words (glSampleMaski)
    // and the minimum sample-shading fraction (glMinSampleShading).
    struct MultisampleRasterState {
        std::array<uint32_t, kMaxSampleMaskWords> sampleMask = {};
        float minSampleShading = 0.0f;
        bool equal(const MultisampleRasterState& o) const {
            return sampleMask == o.sampleMask &&
                   minSampleShading == o.minSampleShading;
        }
    };
    // Provoking vertex convention (SPEC §11, glProvokingVertex). One of
    // GL_FIRST_VERTEX_CONVENTION / GL_LAST_VERTEX_CONVENTION.
    struct ProvokingVertexState {
        GLenum mode = 0x8E4E; // GL_LAST_VERTEX_CONVENTION (GL default)
        bool equal(const ProvokingVertexState& o) const { return mode == o.mode; }
    };
    // Color clamping (SPEC §15.2.3, glClampColor). Only GL_CLAMP_READ_COLOR is a
    // core target; `readColor` is GL_TRUE / GL_FALSE / GL_FIXED_ONLY.
    struct ClampColorState {
        GLenum readColor = 0x891D; // GL_FIXED_ONLY (GL default)
        bool equal(const ClampColorState& o) const { return readColor == o.readColor; }
    };

    struct TextureUnitState {
        std::unordered_map<GLenum, GLObjectName> bound; // target -> name
        bool equal(const TextureUnitState& o) const { return bound == o.bound; }
    };
    std::vector<TextureUnitState> texUnits_;
    std::vector<TextureUnitState> texUnitsApplied_;
    uint32_t activeTextureUnit_ = 0;
    uint32_t activeTextureApplied_ = 0;
    bool textureUnitsDirty_ = false;

    std::unordered_map<GLenum, bool> capsCurrent_;
    std::unordered_map<GLenum, bool> capsApplied_;
    bool capsDirty_ = false;

    // Per-unit sampler-object bindings (SPEC §8.2). Parallel to texUnits_: one
    // bound sampler name per texture unit, 0 when no sampler is bound.
    std::vector<GLObjectName> samplerBound_;
    std::vector<GLObjectName> samplerBoundApplied_;
    bool samplerUnitsDirty_ = false;

    GLObjectName activeProgram_ = 0;
    GLObjectName activeProgramApplied_ = 0;
    bool programDirty_ = false;

    GLObjectName boundProgramPipeline_ = 0;
    GLObjectName boundProgramPipelineApplied_ = 0;
    bool programPipelineDirty_ = false;

    BlendState blend_, blendApplied_;
    BlendColorState blendColor_, blendColorApplied_;
    DepthState depth_, depthApplied_;
    DepthRangeState depthRange_, depthRangeApplied_;
    StencilState stencil_, stencilApplied_;
    RasterState raster_, rasterApplied_;
    RasterScalarState rasterScalar_, rasterScalarApplied_;
    PixelStoreState pixel_, pixelApplied_;
    ViewportState viewport_, viewportApplied_;
    ScissorBoxState scissor_, scissorApplied_;
    ClearColorState clearColor_, clearColorApplied_;
    ClearDepthState clearDepth_, clearDepthApplied_;
    FramebufferBufferState fbBuffers_, fbBuffersApplied_;
    LogicOpState logicOp_, logicOpApplied_;
    ColorMaskState colorMask_, colorMaskApplied_;
    SampleCoverageState sampleCoverage_, sampleCoverageApplied_;
    PrimitiveRestartState primitiveRestart_, primitiveRestartApplied_;
    PolygonModeState polygonMode_, polygonModeApplied_;
    MultisampleRasterState multisampleRaster_, multisampleRasterApplied_;
    ProvokingVertexState provokingVertex_, provokingVertexApplied_;
    ClampColorState clampColor_, clampColorApplied_;
};

} // namespace glcompat
