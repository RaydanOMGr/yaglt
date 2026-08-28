#include "glcompat/core/capabilities_table.hpp"
#include "glcompat/core/log.hpp"

namespace glcompat {

std::string CapabilityTable::featureName(Feature feature) const {
    switch (feature) {
    case Feature::BufferObjects: return "BufferObjects";
    case Feature::ImmutableBufferStorage: return "ImmutableBufferStorage";
    case Feature::TextureObjects: return "TextureObjects";
    case Feature::ImmutableTextureStorage: return "ImmutableTextureStorage";
    case Feature::TextureMultisample: return "TextureMultisample";
    case Feature::TextureViews: return "TextureViews";
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
    case Feature::SamplerObjects: return "SamplerObjects";
    case Feature::FeatureCount: return "FeatureCount";
    }
    return "Unknown";
}

void CapabilityTable::report() const {
    using glcompat::log;
    using glcompat::LogCategory;
    using glcompat::LogLevel;

    int native = 0, emulated = 0, unsupported = 0, other = 0;
    for (size_t i = 0; i < table_.size(); ++i) {
        auto f = static_cast<Feature>(i);
        auto s = table_[i];
        const char* cls = "Unknown";
        switch (s) {
        case FeatureSupport::Native: cls = "Native"; ++native; break;
        case FeatureSupport::Emulated: cls = "Emulated"; ++emulated; break;
        case FeatureSupport::Unsupported: cls = "Unsupported"; ++unsupported; break;
        default: ++other; break;
        }
        log(LogCategory::Emulation, LogLevel::Debug)
            << "feature " << featureName(f) << " -> " << cls;
    }
    log(LogCategory::Emulation, LogLevel::Debug)
        << "capability summary: native=" << native << " emulated=" << emulated
        << " unsupported=" << unsupported << " other=" << other;
}

} // namespace glcompat
