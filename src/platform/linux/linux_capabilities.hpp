#pragma once

#include "glcompat/core/platform.hpp"
#include <string>

namespace glcompat {

// Real Linux platform capabilities for headless development/testing.
// Android-specific logic lives under src/platform/android and is never
// compiled into the Linux build. SDK-dependent branching is isolated here.
class LinuxCapabilities : public IPlatformCapabilities {
public:
    PlatformOs os() const override { return PlatformOs::Linux; }
    int androidSdkVersion() const override { return 0; }

    // On Linux we rely on POSIX shared memory (shm_open) for any shared
    // resource needs; report available.
    bool supportsSharedMemory() const override { return true; }

    std::string describe() const override {
        return "LinuxCapabilities(os=Linux, sdk=0)";
    }
};

} // namespace glcompat
