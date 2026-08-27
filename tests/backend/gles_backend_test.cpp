#include "test_framework.hpp"

#include "glcompat/backend/gles/gles_backend.hpp"
#include "src/backend/gles/gles_resources.hpp"
#include "glcompat/core/backend.hpp"
#include "glcompat/core/capabilities.hpp"

using namespace glcompat;

// The GLES backend resolves its driver at runtime. On this headless Linux box
// libGLESv2 is absent, so initialize() returns false and no driver calls are
// made. Where a driver exists (Android, or a Mesa GLES build) the same binary
// initializes for real. The test passes in both cases: it never fakes support.
TEST_CASE("gles_backend_reports_gles_api") {
    GLESBackend backend;
    EXPECT_EQ(backend.api(), BackendApi::GLES);
}

TEST_CASE("gles_backend_initialize_is_honest_and_safe") {
    GLESBackend backend;
    bool ok = backend.initialize();
    // Either a real driver initialized it, or it failed cleanly. Both legal.
    if (ok) {
        EXPECT_EQ(backend.capabilities().getFeatureSupport(Feature::BufferObjects),
                  FeatureSupport::Native);
        {
            auto buf = backend.resourceFactory().createBuffer();
            auto* gb = static_cast<GLESBackendBuffer*>(buf.get());
            EXPECT_NE(gb->handle, 0u);
        }
        // Resources must be released while the EGL context is still alive; only
        // then tear the backend down. Keeping a resource across shutdown would
        // make its destructor call the driver on a terminated context.
        backend.shutdown();
    } else {
        // No driver in this environment: default capabilities, no crash.
        EXPECT_EQ(backend.capabilities().getFeatureSupport(Feature::BufferObjects),
                  FeatureSupport::Unsupported);
    }
}
