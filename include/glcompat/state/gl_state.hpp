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
    // GL 4.6 core guarantees at least 8 draw buffers (MAX_DRAW_BUFFERS). The
    // frontend tracks a fixed, generous table so per-buffer indices stay stable
    // for indexed blending (SPEC §15.3 / §17.3.4).
    static constexpr uint32_t kMaxDrawBuffers = 8;

    GLStateTracker();

    // --- Capabilities (glEnable / glDisable) ---
    bool setCapability(GLenum cap, bool enabled);
    bool isCapabilityEnabled(GLenum cap) const;

    // --- Indexed capabilities (glEnablei / glDisablei / glIsEnabledi, SPEC §10.3.1) ---
    bool setIndexedCapability(GLenum cap, uint32_t index, bool enabled);
    bool isIndexedCapabilityEnabled(GLenum cap, uint32_t index, bool* enabled) const;

    // --- Hints (glHint, SPEC §21.1.1) ---
    bool setHint(GLenum target, GLenum mode);
    GLenum getHint(GLenum target) const;

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

    // --- Indexed blending (SPEC §15.3 / §17.3.4) ---
    // Per-draw-buffer blend factors/equations. `buf` selects the draw-buffer
    // slot; buffer 0 is the target of the non-indexed glBlendFunc /
    // glBlendEquation setters. The caller validates `buf` < kMaxDrawBuffers
    // (GL_INVALID_VALUE) before invoking these.
    bool setBlendFuncSeparatei(uint32_t buf, GLenum srcRGB, GLenum dstRGB,
                               GLenum srcAlpha, GLenum dstAlpha);
    bool setBlendEquationSeparatei(uint32_t buf, GLenum modeRGB,
                                   GLenum modeAlpha);

    // --- Depth ---
    bool setDepthFunc(GLenum func);
    bool setDepthMask(bool enabled);
    bool setDepthRange(double nearVal, double farVal);
    // Per-viewport depth range (glDepthRangeIndexed / glDepthRangeArrayv,
    // SPEC §13.5.2). `index` selects the viewport slot; index 0 is the target
    // of the non-indexed `setDepthRange`. The caller validates `index` <
    // kMaxViewports (GL_INVALID_VALUE) before invoking this.
    bool setDepthRangeIndexed(uint32_t index, double nearVal, double farVal);

    // --- Stencil ---
    bool setStencilFunc(GLenum func, GLint ref, GLuint mask);
    bool setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
    bool setStencilMask(GLuint mask);
    bool setStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
    bool setStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
    bool setStencilMaskSeparate(GLenum face, GLuint mask);

    // --- Color write mask (SPEC §17.3.6, glColorMask / glColorMaski) ---
    // The non-indexed `glColorMask` sets every draw buffer to the same mask; the
    // indexed `glColorMaski` sets a single draw-buffer slot. Each channel is an
    // independent boolean pushed only when the set of masked channels changes.
    bool setColorMask(bool r, bool g, bool b, bool a);
    bool setColorMaski(uint32_t buf, bool r, bool g, bool b, bool a);

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
    bool setPolygonOffsetClamp(float factor, float units, float clamp);

    // --- Point parameters (SPEC §10.2, glPointParameter{i,f,iv,fv}) ---
    // The caller validates pname/value (GL_INVALID_ENUM / GL_INVALID_VALUE) and
    // only invokes the matching setter. Each stores the field and returns true.
    bool setPointParameteri(GLenum pname, GLint param);
    bool setPointParameterf(GLenum pname, GLfloat param);
    bool setPointParameteriv(GLenum pname, const GLint* params);
    bool setPointParameterfv(GLenum pname, const GLfloat* params);
    float pointSizeMin() const { return pointParam_.sizeMin; }
    float pointSizeMax() const { return pointParam_.sizeMax; }
    float pointFadeThreshold() const { return pointParam_.fadeThreshold; }
    GLenum pointSpriteCoordOrigin() const { return pointParam_.spriteCoordOrigin; }

    // --- Patch parameters (SPEC §10.6, glPatchParameter{i,fv}) ---
    // The caller validates pname/value (GL_INVALID_ENUM / GL_INVALID_VALUE) and
    // only invokes the matching setter. Each stores the field and returns true
    // only when the relevant field actually changed.
    bool setPatchParameteri(GLenum pname, GLint value);
    bool setPatchParameterfv(GLenum pname, const GLfloat* values);
    uint32_t patchVertices() const { return patch_.patchVertices; }

    // --- Clip control (SPEC §12.1, glClipControl) ---
    // origin is GL_LOWER_LEFT / GL_UPPER_LEFT; depth is GL_NEGATIVE_ONE_TO_ONE /
    // GL_ZERO_TO_ONE. GL defaults: GL_LOWER_LEFT + GL_NEGATIVE_ONE_TO_ONE. The
    // caller validates both enums (GL_INVALID_ENUM) before invoking the setter.
    bool setClipControl(GLenum origin, GLenum depth);
    GLenum clipOrigin() const { return clip_.origin; }
    GLenum clipDepthMode() const { return clip_.depth; }


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

    // --- Viewport (glViewport, SPEC §10) ---
    // Index 0 is the default viewport set by glViewport; indexed variants
    // (glViewportIndexedf/fv, SPEC §10.3.1) set arbitrary viewports.
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
    static constexpr uint32_t kMaxViewports = 16;
    bool setViewport(GLint x, GLint y, GLsizei width, GLsizei height);
    bool setViewportIndexed(GLuint index, GLint x, GLint y, GLsizei width,
                            GLsizei height);
    // glViewportArrayv (SPEC §13.5.2): sets `count` contiguous viewports
    // starting at `first` from a packed float array (4 floats per viewport).
    // Returns true when any slot changed. The caller validates `first`+`count`
    // <= kMaxViewports, `count` > 0 and `v` != nullptr (GL_INVALID_VALUE).
    bool setViewportIndexedv(uint32_t first, uint32_t count, const GLfloat* v);

    // --- Scissor box (glScissor); the scissor test is GL_SCISSOR_TEST cap ---
    bool setScissor(GLint x, GLint y, GLsizei width, GLsizei height);
    bool setScissorIndexed(GLuint index, GLint x, GLint y, GLsizei width,
                           GLsizei height);
    // glScissorArrayv (SPEC §13.5.2): sets `count` contiguous scissor boxes
    // starting at `first` from a packed int array (4 ints per box). Returns true
    // when any slot changed. The caller validates `first`+`count` <=
    // kMaxViewports, `count` > 0 and `v` != nullptr (GL_INVALID_VALUE).
    bool setScissorIndexedv(uint32_t first, uint32_t count, const GLint* v);

    const ViewportState& viewport(uint32_t index) const {
        return viewport_[index < kMaxViewports ? index : 0];
    }
    const ScissorBoxState& scissor(uint32_t index) const {
        return scissor_[index < kMaxViewports ? index : 0];
    }

    // --- Clear values (glClearColor / glClearDepth, SPEC §2.1) ---
    bool setClearColor(float r, float g, float b, float a);
    bool setClearDepth(double d);
    bool setClearStencil(int s);

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

    // Multi-bind samplers (`glBindSamplers`, SPEC §8.2 / ARB_multi_bind). Binds
    // an array of samplers to consecutive units [first, first+count). `names`
    // may be null (treated as all-zero, i.e. unbind every touched unit). Returns
    // true when any unit's binding changed. Out-of-range bounds return false
    // (the caller reports the GL error). The caller is responsible for per-unit
    // name validation and passes the already resolved bindings, so entries it
    // rejected must repeat the unit's current binding.
    bool setSamplerBindings(uint32_t first, uint32_t count,
                            const GLObjectName* names);

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
    struct StencilFaceState {
        GLenum func = 0x0207;     // GL_ALWAYS
        GLint ref = 0;
        GLuint mask = ~0u;
        GLenum sfail = 0x1E00;    // GL_KEEP
        GLenum dpfail = 0x1E00;   // GL_KEEP
        GLenum dppass = 0x1E00;   // GL_KEEP
        GLuint writeMask = ~0u;
        bool equal(const StencilFaceState& o) const {
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
        float polygonOffsetClamp = 0.0f;
        bool equal(const RasterScalarState& o) const {
            return pointSize == o.pointSize && lineWidth == o.lineWidth &&
                   polygonOffsetFactor == o.polygonOffsetFactor &&
                   polygonOffsetUnits == o.polygonOffsetUnits &&
                   polygonOffsetClamp == o.polygonOffsetClamp;
        }
    };
    // Point parameters (SPEC §10.2, glPointParameter*). sizeMin/sizeMax/
    // fadeThreshold are non-negative floats (GL default 0/1/0); spriteCoordOrigin
    // is GL_UPPER_LEFT (GL default) or GL_LOWER_LEFT.
    struct PointParamState {
        float sizeMin = 0.0f;
        float sizeMax = 1.0f;
        float fadeThreshold = 0.0f;
        GLenum spriteCoordOrigin = 0x8CA2; // GL_UPPER_LEFT (GL default)
        bool equal(const PointParamState& o) const {
            return sizeMin == o.sizeMin && sizeMax == o.sizeMax &&
                   fadeThreshold == o.fadeThreshold &&
                   spriteCoordOrigin == o.spriteCoordOrigin;
        }
    };
    // Patch parameters (SPEC §10.6, glPatchParameter{i,fv}). patchVertices is the
    // per-patch vertex count (GL default 3); outer/inner levels are the default
    // tessellation levels (GL default all 1.0). std::array gives value semantics
    // and == comparison for cheap change detection.
    struct PatchParameterState {
        uint32_t patchVertices = 3;
        std::array<float, 4> patchOuterLevel = {1.0f, 1.0f, 1.0f, 1.0f};
        std::array<float, 2> patchInnerLevel = {1.0f, 1.0f};
        bool equal(const PatchParameterState& o) const {
            return patchVertices == o.patchVertices &&
                   patchOuterLevel == o.patchOuterLevel &&
                   patchInnerLevel == o.patchInnerLevel;
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
    struct ClearStencilState {
        int stencil = 0;
        bool equal(const ClearStencilState& o) const {
            return stencil == o.stencil;
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

    // Indexed capabilities (glEnablei/glDisablei). cap -> (index -> enabled).
    std::unordered_map<GLenum, std::unordered_map<uint32_t, bool>> indexedCapsCurrent_;
    std::unordered_map<GLenum, std::unordered_map<uint32_t, bool>> indexedCapsApplied_;
    bool indexedCapsDirty_ = false;

    // Quality hints (SPEC §21.1.1, glHint). target -> mode. Pushed on flush.
    std::unordered_map<GLenum, GLenum> hints_;
    std::unordered_map<GLenum, GLenum> hintsApplied_;
    bool hintsDirty_ = false;

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

    // Per-draw-buffer blend factors/equations. Index 0 mirrors the non-indexed
    // glBlendFunc / glBlendEquation setters; apply() pushes buffer 0 through the
    // non-indexed sink methods and buffers 1..n through the indexed variants so
    // backends that only support single-buffer blend keep working (SPEC §15.3).
    std::vector<BlendState> blendBuf_;
    std::vector<BlendState> blendBufApplied_;
    BlendColorState blendColor_, blendColorApplied_;
    DepthState depth_, depthApplied_;
    DepthRangeState depthRange_[kMaxViewports] = {};
    DepthRangeState depthRangeApplied_[kMaxViewports] = {};
    StencilFaceState stencilFront_, stencilBack_;
    StencilFaceState stencilFrontApplied_, stencilBackApplied_;
    RasterState raster_, rasterApplied_;
    RasterScalarState rasterScalar_, rasterScalarApplied_;
    PointParamState pointParam_, pointParamApplied_;
    // glPatchParameter{i,fv} (SPEC §10.6). patchVertices / outer / inner levels.
    PatchParameterState patch_, patchApplied_;
    // glClipControl (SPEC §12.1). origin/depth select the clip-volume origin and
    // depth range mapping; GL default GL_LOWER_LEFT + GL_NEGATIVE_ONE_TO_ONE.
    struct ClipControlState {
        GLenum origin = 0x8CA1; // GL_LOWER_LEFT (GL default)
        GLenum depth = 0x8E27;  // GL_NEGATIVE_ONE_TO_ONE (GL default)
        bool equal(const ClipControlState& o) const {
            return origin == o.origin && depth == o.depth;
        }
    };
    ClipControlState clip_, clipApplied_;
    PixelStoreState pixel_, pixelApplied_;
    ViewportState viewport_[kMaxViewports] = {};
    ViewportState viewportApplied_[kMaxViewports] = {};
    ScissorBoxState scissor_[kMaxViewports] = {};
    ScissorBoxState scissorApplied_[kMaxViewports] = {};
    ClearColorState clearColor_, clearColorApplied_;
    ClearDepthState clearDepth_, clearDepthApplied_;
    ClearStencilState clearStencil_, clearStencilApplied_;
    FramebufferBufferState fbBuffers_, fbBuffersApplied_;
    LogicOpState logicOp_, logicOpApplied_;
    // Per-draw-buffer color write masks. Index 0 is the target of the
    // non-indexed `glColorMask`; `apply()` pushes buffer 0 through the
    // single-buffer `colorMask` sink and buffers 1..n through `colorMaski`, so
    // backends without per-buffer color mask keep working (SPEC §15.3).
    std::vector<ColorMaskState> colorMask_;
    std::vector<ColorMaskState> colorMaskApplied_;
    SampleCoverageState sampleCoverage_, sampleCoverageApplied_;
    PrimitiveRestartState primitiveRestart_, primitiveRestartApplied_;
    PolygonModeState polygonMode_, polygonModeApplied_;
    MultisampleRasterState multisampleRaster_, multisampleRasterApplied_;
    ProvokingVertexState provokingVertex_, provokingVertexApplied_;
    ClampColorState clampColor_, clampColorApplied_;
};

} // namespace glcompat
