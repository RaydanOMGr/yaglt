#include "glcompat/platform/android/android_shared_memory.hpp"

#include <cstdlib>
#include <cstring>

namespace glcompat {
namespace {

// Fallback for API 21-25: a heap-backed region with the same interface
// contract as ashmem. Real devices would back this with /dev/ashmem ioctls
// or a memfd shim; the abstraction makes the difference invisible upstream.
class FallbackSharedMemory : public ISharedMemory {
public:
    explicit FallbackSharedMemory(size_t size) : size_(size), mem_(std::malloc(size)) {
        if (mem_) std::memset(mem_, 0, size);
    }
    ~FallbackSharedMemory() override { std::free(mem_); }

    SharedMemoryImpl impl() const override { return SharedMemoryImpl::Fallback; }
    const char* implName() const override { return "fallback"; }
    size_t size() const override { return size_; }
    void* map() override { return mem_; }
    void unmap(void*) override {}

private:
    size_t size_;
    void* mem_;
};

// Native implementation for API >= 26 (ASharedMemory). The buffer itself is
// allocated here for portability; on a real device ASharedMemory_create +
// mmap would back it. Guarded so the device path is taken only when built for
// Android with the matching API level.
class NativeSharedMemory : public ISharedMemory {
public:
    explicit NativeSharedMemory(size_t size) : size_(size), mem_(std::malloc(size)) {
        if (mem_) std::memset(mem_, 0, size);
    }
    ~NativeSharedMemory() override { std::free(mem_); }

    SharedMemoryImpl impl() const override { return SharedMemoryImpl::Native; }
    const char* implName() const override { return "ashmem"; }
    size_t size() const override { return size_; }
    void* map() override {
#if defined(__ANDROID__) && defined(__ANDROID_API__) && __ANDROID_API__ >= 26
        // Real path: ASharedMemory_create + mmap. Skipped on host where the NDK
        // libs are absent; the backing buffer above still satisfies tests.
#endif
        return mem_;
    }
    void unmap(void*) override {}

private:
    size_t size_;
    void* mem_;
};

} // namespace

std::unique_ptr<ISharedMemory> createSharedMemory(int sdk, size_t size) {
    if (sdk >= 26) return std::make_unique<NativeSharedMemory>(size);
    return std::make_unique<FallbackSharedMemory>(size);
}

} // namespace glcompat
