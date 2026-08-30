#pragma once

#include "mock_resources.hpp"

#include "glcompat/core/factory.hpp"

namespace glcompat {

class MockResourceFactory : public IResourceFactory {
public:
    std::unique_ptr<BackendBuffer> createBuffer() override {
        auto r = std::make_unique<MockBuffer>();
        r->id = ++counter_;
        lastCreatedBuffer = r.get();
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
    std::unique_ptr<BackendSampler> createSampler() override {
        auto r = std::make_unique<MockSampler>();
        r->id = ++counter_;
        return r;
    }
    std::unique_ptr<BackendTransformFeedback> createTransformFeedback() override {
        auto r = std::make_unique<MockTransformFeedback>();
        r->id = ++counter_;
        return r;
    }
    std::unique_ptr<BackendQuery> createQuery() override {
        auto r = std::make_unique<MockQuery>();
        r->id = ++counter_;
        lastCreatedQuery = r.get();
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
        lastCreatedProgram = r.get();
        return r;
    }
    // Test helper: the most recently created MockProgram (nullptr before any
    // program is created). Lets tests configure/observe program state directly.
    MockProgram* lastCreatedProgram = nullptr;
    // Test helper: the most recently created MockQuery (nullptr before any query
    // is created). Lets tests configure/observe query state directly.
    MockQuery* lastCreatedQuery = nullptr;
    // Test helper: the most recently created MockBuffer (nullptr before any
    // buffer is created). Lets tests observe buffer-backend calls directly.
    MockBuffer* lastCreatedBuffer = nullptr;

private:
    int counter_ = 0;
};

} // namespace glcompat
