#pragma once

#include "glcompat/core/capabilities_table.hpp"
#include "glcompat/core/platform.hpp"

namespace glcompat {

// Populate a capability table with a believable mock-backend profile.
// The profile mirrors a reasonable GLES 3.1 baseline: core features native,
// the geometry/tessellation stages (no GLES equivalent) reported honestly as
// unsupported, and some features marked emulated to exercise the emulation
// code paths in tests. Compute shaders are native in GLES 3.1+ and reported
// Native here.
inline void populateMockCapabilities(CapabilityTable& table) {
    using F = Feature;
    using S = FeatureSupport;

    table.set(F::BufferObjects, S::Native);
    table.set(F::ImmutableBufferStorage, S::Native);
    table.set(F::TextureObjects, S::Native);
    table.set(F::ImmutableTextureStorage, S::Native);
    table.set(F::TextureMultisample, S::Native);
    table.set(F::ShaderObjects, S::Native);
    table.set(F::ProgramObjects, S::Native);
    table.set(F::GeometryShaders, S::Unsupported);
    table.set(F::TessellationShaders, S::Unsupported);
    // Compute shaders are native in GLES 3.1+ (the real GLES backend reports
    // them Native there), so the mock mirrors that baseline rather than lying
    // that they are unavailable. Geometry/tessellation have no GLES equivalent
    // and remain honestly Unsupported.
    table.set(F::ComputeShaders, S::Native);
    table.set(F::VertexArrayObjects, S::Native);
    table.set(F::InstancedRendering, S::Native);
    table.set(F::VertexAttribDivisor, S::Native);
    table.set(F::MultiDraw, S::Native);
    table.set(F::DrawRangeElements, S::Native);
    table.set(F::DrawElementsBaseVertex, S::Native);
    table.set(F::FramebufferObjects, S::Native);
    table.set(F::RenderbufferObjects, S::Native);
    table.set(F::UniformBufferObjects, S::Native);
    table.set(F::ShaderStorageBufferObjects, S::Native);
    table.set(F::TransformFeedback, S::Native);
    table.set(F::ImageLoadStore, S::Unsupported);
    table.set(F::IndirectDrawing, S::Native);
    table.set(F::ProgramPipelines, S::Emulated);
    table.set(F::DirectStateAccess, S::Emulated);
    table.set(F::SamplerObjects, S::Native);
    table.set(F::Subroutines, S::Emulated);
    table.set(F::Queries, S::Native);
    table.set(F::SyncObjects, S::Native);
    table.set(F::LogicOp, S::Native);
}

// Mock platform: used for headless Linux testing. On Android the real
// platform implementation is supplied instead; this one reports Linux.
class MockPlatformCapabilities : public IPlatformCapabilities {
public:
    explicit MockPlatformCapabilities(PlatformOs os = PlatformOs::Linux, int sdk = 0)
        : os_(os), sdk_(sdk) {}

    PlatformOs os() const override { return os_; }
    int androidSdkVersion() const override { return sdk_; }
    bool supportsSharedMemory() const override { return true; }
    std::string describe() const override {
        return "MockPlatform(os=" + std::to_string(static_cast<int>(os_)) +
               ", sdk=" + std::to_string(sdk_) + ")";
    }

private:
    PlatformOs os_;
    int sdk_;
};

} // namespace glcompat
