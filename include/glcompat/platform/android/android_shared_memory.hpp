#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace glcompat {

// Shared-memory abstraction (SPEC §5). On Android, API 26+ exposes
// ASharedMemory natively; older API levels (down to SDK 21) require a fallback.
// The renderer only ever sees ISharedMemory; the concrete implementation is
// chosen once at init, so SDK-version branching never reaches the frontend.
enum class SharedMemoryImpl {
    Native,   // API >= 26 (ASharedMemory / ashmem)
    Fallback  // API 21-25 emulated fallback
};

class ISharedMemory {
public:
    virtual ~ISharedMemory() = default;

    virtual SharedMemoryImpl impl() const = 0;
    virtual const char* implName() const = 0;
    virtual size_t size() const = 0;

    // Map the region for use. Returns nullptr on failure.
    virtual void* map() = 0;
    virtual void unmap(void* p) = 0;
};

// Select the implementation for the given Android SDK level.
// API >= 26 -> Native; older -> Fallback (SPEC §5 example).
std::unique_ptr<ISharedMemory> createSharedMemory(int sdk, size_t size);

} // namespace glcompat
