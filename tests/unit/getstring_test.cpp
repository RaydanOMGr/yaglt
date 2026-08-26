#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

namespace {
const char* str(const GLubyte* p) { return reinterpret_cast<const char*>(p); }
} // namespace

TEST_CASE("gl_getstring_vendor_renderer_version") {
    auto backend = std::make_unique<MockBackend>();
    backend->initialize();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    EXPECT_EQ(std::string(str(glGetString(GL_VENDOR))), std::string("YAGLT"));
    EXPECT_EQ(std::string(str(glGetString(GL_RENDERER))), std::string("YAGLT"));
    EXPECT_EQ(std::string(str(glGetString(GL_VERSION))),
              std::string("4.6.0 Compatibility Profile YAGLT"));
    EXPECT_EQ(std::string(str(glGetString(GL_SHADING_LANGUAGE_VERSION))),
              std::string("4.60"));
    EXPECT_EQ(std::string(str(glGetString(GL_EXTENSIONS))), std::string(""));

    // A valid query must not leave an error pending.
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    glcompat::setCurrentContext(nullptr);
}

TEST_CASE("gl_getstring_invalid_name_yields_error") {
    auto backend = std::make_unique<MockBackend>();
    backend->initialize();
    Context ctx(*backend);
    glcompat::setCurrentContext(&ctx);

    EXPECT_EQ(glGetString(0xDEAD), nullptr);
    EXPECT_EQ(glGetError(), GL_INVALID_ENUM);
    glcompat::setCurrentContext(nullptr);
}

TEST_CASE("gl_getstring_no_context_returns_null") {
    glcompat::setCurrentContext(nullptr);
    EXPECT_EQ(glGetString(GL_VENDOR), nullptr);
}
