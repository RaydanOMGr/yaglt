#include "glcompat/core/capabilities_table.hpp"

namespace glcompat {

std::string CapabilityTable::featureName(Feature feature) const {
    switch (feature) {
    case Feature::BufferObjects: return "BufferObjects";
    case Feature::ImmutableBufferStorage: return "ImmutableBufferStorage";
    case Feature::TextureObjects: return "TextureObjects";
    case Feature::ImmutableTextureStorage: return "ImmutableTextureStorage";
    case Feature::TextureMultisample: return "TextureMultisample";
    case Feature::ShaderObjects: return "ShaderObjects";
    case Feature::ProgramObjects: return "ProgramObjects";
    case Feature::GeometryShaders: return "GeometryShaders";
    case Feature::TessellationShaders: return "TessellationShaders";
    case Feature::ComputeShaders: return "ComputeShaders";
    case Feature::VertexArrayObjects: return "VertexArrayObjects";
    case Feature::InstancedRendering: return "InstancedRendering";
    case Feature::FramebufferObjects: return "FramebufferObjects";
    case Feature::RenderbufferObjects: return "RenderbufferObjects";
    case Feature::UniformBufferObjects: return "UniformBufferObjects";
    case Feature::ShaderStorageBufferObjects: return "ShaderStorageBufferObjects";
    case Feature::TransformFeedback: return "TransformFeedback";
    case Feature::ImageLoadStore: return "ImageLoadStore";
    case Feature::IndirectDrawing: return "IndirectDrawing";
    case Feature::ProgramPipelines: return "ProgramPipelines";
    case Feature::DirectStateAccess: return "DirectStateAccess";
    case Feature::FeatureCount: return "FeatureCount";
    }
    return "Unknown";
}

} // namespace glcompat
