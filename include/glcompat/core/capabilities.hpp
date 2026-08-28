#pragma once

#include <string>

namespace glcompat {

// How a given OpenGL feature is provided by the active backend/configuration.
enum class FeatureSupport {
    Native,     // backend provides it directly
    Emulated,   // provided via emulation (shader/CPU/resource)
    Unsupported // not available; must be reported honestly
};

// OpenGL feature identifiers tracked by the capability system.
// Keep this list aligned with docs/feature-matrix.md.
enum class Feature {
    // Buffers
    BufferObjects,
    ImmutableBufferStorage,
    // Textures
    TextureObjects,
    ImmutableTextureStorage,
    TextureMultisample,
    TextureViews,
    // Shaders
    ShaderObjects,
    ProgramObjects,
    GeometryShaders,
    TessellationShaders,
    ComputeShaders,
    // Vertex
    VertexArrayObjects,
    InstancedRendering,
    VertexAttribDivisor,
    MultiDraw,
    DrawRangeElements,
    DrawElementsBaseVertex,
    // Framebuffers
    FramebufferObjects,
    RenderbufferObjects,
    // Modern
    UniformBufferObjects,
    ShaderStorageBufferObjects,
    TransformFeedback,
    ImageLoadStore,
    IndirectDrawing,
    ProgramPipelines,
    DirectStateAccess,
    SamplerObjects,
    // Shaders: subroutines (SPEC §7.9, GLES 3.1+)
    Subroutines,
    // Queries / sync (SPEC §4 / §19 / §20)
    Queries,
    SyncObjects,
    // Per-fragment color logic op (SPEC §17.3.4)
    LogicOp,
    // Conditional rendering (SPEC §10.11, glBeginConditionalRender /
    // glEndConditionalRender). Core in desktop GL 4.6 (ARB_conditional_render_*
    // and *_inverted); on GLES only available via GL_NV_conditional_render.
    ConditionalRendering,
    // Sentinel
    FeatureCount
};

// Centralized capability query interface.
// Frontend asks this instead of checking GLES version, Android SDK, extensions.
class ICapabilities {
public:
    virtual ~ICapabilities() = default;

    virtual FeatureSupport getFeatureSupport(Feature feature) const = 0;

    bool isSupported(Feature feature) const {
        FeatureSupport s = getFeatureSupport(feature);
        return s == FeatureSupport::Native || s == FeatureSupport::Emulated;
    }

    virtual std::string featureName(Feature feature) const = 0;
};

} // namespace glcompat
