#pragma once

#include <cstdint>
#include <string>

namespace glcompat {

// Backend graphics API kind. Frontend never branches on the concrete value
// except where a backend is explicitly selected at startup.
enum class BackendApi {
    Unknown,
    GLES,
    Vulkan,
    DesktopGL,
    Mock
};

// Platform OS kind.
enum class PlatformOs {
    Unknown,
    Android,
    Linux
};

// Platform capability abstraction. Android SDK differences and other
// platform-specific behavior are resolved here, once, at init time.
class IPlatformCapabilities {
public:
    virtual ~IPlatformCapabilities() = default;

    virtual PlatformOs os() const = 0;

    // 0 when not Android.
    virtual int androidSdkVersion() const = 0;

    virtual bool supportsSharedMemory() const = 0;

    virtual std::string describe() const = 0;
};

} // namespace glcompat
