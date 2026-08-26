#pragma once

#include "glcompat/backend/gles/gles_loader.hpp"
#include "glcompat/core/factory.hpp"
#include "src/backend/gles/gles_resources.hpp"

namespace glcompat {

// Creates real GLES objects via the dynamically loaded driver entry points.
class GLESResourceFactory : public IResourceFactory {
public:
    explicit GLESResourceFactory(GLESLibPtr lib) : lib_(lib) {}

    std::unique_ptr<BackendBuffer> createBuffer() override {
        GLuint h = 0;
        lib_->glGenBuffers(1, &h);
        return std::make_unique<GLESBackendBuffer>(lib_, h);
    }
    std::unique_ptr<BackendTexture> createTexture() override {
        GLuint h = 0;
        lib_->glGenTextures(1, &h);
        return std::make_unique<GLESBackendTexture>(lib_, h);
    }
    std::unique_ptr<BackendRenderbuffer> createRenderbuffer() override {
        GLuint h = 0;
        lib_->glGenRenderbuffers(1, &h);
        return std::make_unique<GLESBackendRenderbuffer>(lib_, h);
    }
    std::unique_ptr<BackendFramebuffer> createFramebuffer() override {
        GLuint h = 0;
        lib_->glGenFramebuffers(1, &h);
        return std::make_unique<GLESBackendFramebuffer>(lib_, h);
    }
    std::unique_ptr<BackendVertexArray> createVertexArray() override {
        GLuint h = 0;
        lib_->glGenVertexArrays(1, &h);
        return std::make_unique<GLESBackendVertexArray>(lib_, h);
    }
    std::unique_ptr<BackendSampler> createSampler() override {
        return std::make_unique<GLESBackendSampler>(lib_);
    }
    std::unique_ptr<BackendTransformFeedback> createTransformFeedback() override {
        return std::make_unique<GLESBackendTransformFeedback>(lib_);
    }
    std::unique_ptr<BackendQuery> createQuery() override {
        return std::make_unique<GLESBackendQuery>(lib_);
    }
    std::unique_ptr<BackendShader> createShader(uint32_t stage) override {
        return std::make_unique<GLESBackendShader>(lib_, stage);
    }
    std::unique_ptr<BackendProgram> createProgram() override {
        return std::make_unique<GLESBackendProgram>(lib_);
    }

private:
    GLESLibPtr lib_;
};

} // namespace glcompat
