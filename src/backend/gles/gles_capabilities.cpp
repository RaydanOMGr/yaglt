#include "glcompat/core/capabilities_table.hpp"
#include "glcompat/backend/gles/gles_loader.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

namespace glcompat {

// Populate the capability table from the detected GLES version and extensions.
// Honest: only marks features Native/Emulated when this GLES version actually
// provides them; geometry/tessellation have no GLES equivalent (Unsupported),
// DSA is core only in desktop GL (Unsupported here unless an ext is present).
void populateGLESCapabilities(CapabilityTable& table, const GLESLib& lib) {
    using F = Feature;
    using S = FeatureSupport;

    bool es3 = lib.glesMajor >= 3;
    bool es31 = (lib.glesMajor == 3 && lib.glesMinor >= 1) || lib.glesMajor > 3;

    auto has = [&](const char* ext) -> bool {
        return lib.extensionsString.find(ext) != std::string::npos;
    };

    table.set(F::BufferObjects, S::Native);
    table.set(F::TextureObjects, S::Native);
    table.set(F::VertexArrayObjects, es3 ? S::Native : S::Emulated);
    table.set(F::FramebufferObjects, es3 ? S::Native : S::Emulated);
    table.set(F::RenderbufferObjects, es3 ? S::Native : S::Emulated);
    table.set(F::ShaderObjects, S::Native);
    table.set(F::ProgramObjects, S::Native);
    table.set(F::InstancedRendering, es3 ? S::Native : S::Emulated);
    table.set(F::VertexAttribDivisor, es3 ? S::Native : S::Unsupported);
    table.set(F::MultiDraw, es3 ? S::Native : S::Unsupported);
    table.set(F::DrawRangeElements, es3 ? S::Native : S::Unsupported);
    bool es32 = (lib.glesMajor == 3 && lib.glesMinor >= 2) || lib.glesMajor > 3;
    table.set(F::DrawElementsBaseVertex, es32 ? S::Native : S::Unsupported);

    table.set(F::ImmutableTextureStorage, es3 ? S::Native : S::Emulated);
    table.set(F::ImmutableBufferStorage, es31 ? S::Native : S::Unsupported);
    table.set(F::TextureMultisample, es31 ? S::Native : S::Unsupported);

    table.set(F::UniformBufferObjects, es3 ? S::Native : S::Unsupported);
    table.set(F::ShaderStorageBufferObjects, es31 ? S::Native : S::Unsupported);
    table.set(F::TransformFeedback, es3 ? S::Native : S::Unsupported);
    table.set(F::ImageLoadStore, es31 ? S::Native : S::Unsupported);
    table.set(F::IndirectDrawing, es31 ? S::Native : S::Unsupported);
    table.set(F::ComputeShaders, es31 ? S::Native : S::Unsupported);

    // GLES has no geometry/tessellation stages at all.
    table.set(F::GeometryShaders, S::Unsupported);
    table.set(F::TessellationShaders, S::Unsupported);

    // Separate program pipelines available via EXT on ES3.1-class drivers.
    table.set(F::ProgramPipelines,
              (es31 || has("GL_EXT_separate_shader_objects")) ? S::Emulated : S::Unsupported);
    // Direct state access is desktop GL; GLES uses object-binding semantics.
    table.set(F::DirectStateAccess, has("GL_EXT_direct_state_access")
                                        ? S::Emulated
                                        : S::Unsupported);
    // Sampler objects are core in GLES 3.0 (glBindSampler / glSamplerParameteri).
    table.set(F::SamplerObjects, es3 ? S::Native : S::Unsupported);

    // Occlusion / primitive / timer queries are core in GLES 3.0 (glBeginQuery,
    // glEndQuery, glGetQueryObjectuiv); sync fences are core in GLES 3.0 too.
    table.set(F::Queries, es3 ? S::Native : S::Unsupported);
    table.set(F::SyncObjects, es3 ? S::Native : S::Unsupported);
    // Color logic op (glLogicOp) is core in GLES 3.0.
    table.set(F::LogicOp, es3 ? S::Native : S::Unsupported);
}

} // namespace glcompat
