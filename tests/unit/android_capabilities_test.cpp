#include "test_framework.hpp"

#include "glcompat/platform/android/android_capabilities.hpp"
#include "glcompat/platform/android/android_shared_memory.hpp"

using namespace glcompat;

TEST_CASE("android_capabilities_identity") {
    AndroidCapabilities cap(30);
    EXPECT_EQ(static_cast<int>(cap.os()), static_cast<int>(PlatformOs::Android));
    EXPECT_EQ(cap.androidSdkVersion(), 30);
    EXPECT_TRUE(cap.supportsSharedMemory());
    EXPECT_EQ(cap.describe(), std::string("AndroidCapabilities(os=Android, sdk=30)"));
}

TEST_CASE("android_shared_memory_impl_selection") {
    // API 26+ resolves to the native ASharedMemory path.
    EXPECT_EQ(static_cast<int>(AndroidCapabilities(26).sharedMemoryImpl()),
              static_cast<int>(SharedMemoryImpl::Native));
    EXPECT_EQ(static_cast<int>(AndroidCapabilities(31).sharedMemoryImpl()),
              static_cast<int>(SharedMemoryImpl::Native));

    // Below 26 (down to SDK 21) uses the fallback path.
    EXPECT_EQ(static_cast<int>(AndroidCapabilities(25).sharedMemoryImpl()),
              static_cast<int>(SharedMemoryImpl::Fallback));
    EXPECT_EQ(static_cast<int>(AndroidCapabilities(21).sharedMemoryImpl()),
              static_cast<int>(SharedMemoryImpl::Fallback));
}

TEST_CASE("android_create_shared_memory_native") {
    auto sm = createSharedMemory(30, 128);
    EXPECT_EQ(static_cast<int>(sm->impl()), static_cast<int>(SharedMemoryImpl::Native));
    EXPECT_EQ(std::string(sm->implName()), std::string("ashmem"));
    EXPECT_EQ(sm->size(), static_cast<size_t>(128));
    void* p = sm->map();
    EXPECT_NE(p, nullptr);
    sm->unmap(p);
}

TEST_CASE("android_create_shared_memory_fallback") {
    auto sm = createSharedMemory(21, 64);
    EXPECT_EQ(static_cast<int>(sm->impl()), static_cast<int>(SharedMemoryImpl::Fallback));
    EXPECT_EQ(std::string(sm->implName()), std::string("fallback"));
    EXPECT_EQ(sm->size(), static_cast<size_t>(64));
    void* p = sm->map();
    EXPECT_NE(p, nullptr);
    sm->unmap(p);
}
