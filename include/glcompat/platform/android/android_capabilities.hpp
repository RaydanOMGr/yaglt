#pragma once

#include "glcompat/core/platform.hpp"
#include "glcompat/platform/android/android_shared_memory.hpp"
#include <string>

namespace glcompat {

// Android platform capabilities (SPEC §5/§18). SDK-dependent decisions are
// resolved here once at init; the rest of the project asks this abstraction
// rather than scattering `if (sdk >= 26)` checks through rendering code.
class AndroidCapabilities : public IPlatformCapabilities {
public:
    explicit AndroidCapabilities(int sdk) : sdk_(sdk) {}

    PlatformOs os() const override { return PlatformOs::Android; }
    int androidSdkVersion() const override { return sdk_; }

    // Both the native and fallback paths provide shared memory.
    bool supportsSharedMemory() const override { return true; }

    std::string describe() const override {
        return "AndroidCapabilities(os=Android, sdk=" + std::to_string(sdk_) + ")";
    }

    // Which shared-memory implementation the platform resolves to for sdk_.
    SharedMemoryImpl sharedMemoryImpl() const {
        return sdk_ >= 26 ? SharedMemoryImpl::Native : SharedMemoryImpl::Fallback;
    }

private:
    int sdk_;
};

} // namespace glcompat
