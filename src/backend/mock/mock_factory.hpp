#include "mock_resources.hpp"

#include "glcompat/core/factory.hpp"

namespace glcompat {

class MockResourceFactory : public IResourceFactory {
public:
    std::unique_ptr<BackendBuffer> createBuffer() override {
        auto r = std::make_unique<MockBuffer>();
        r->id = ++counter_;
        return r;
    }
    std::unique_ptr<BackendTexture> createTexture() override {
        auto r = std::make_unique<MockTexture>();
        r->id = ++counter_;
        return r;
    }
    std::unique_ptr<BackendRenderbuffer> createRenderbuffer() override {
        auto r = std::make_unique<MockRenderbuffer>();
        r->id = ++counter_;
        return r;
    }
    std::unique_ptr<BackendFramebuffer> createFramebuffer() override {
        auto r = std::make_unique<MockFramebuffer>();
        r->id = ++counter_;
        return r;
    }
    std::unique_ptr<BackendVertexArray> createVertexArray() override {
        auto r = std::make_unique<MockVertexArray>();
        r->id = ++counter_;
        return r;
    }
    std::unique_ptr<BackendShader> createShader(uint32_t) override {
        auto r = std::make_unique<MockShader>();
        r->id = ++counter_;
        return r;
    }
    std::unique_ptr<BackendProgram> createProgram() override {
        auto r = std::make_unique<MockProgram>();
        r->id = ++counter_;
        return r;
    }

private:
    int counter_ = 0;
};

} // namespace glcompat
