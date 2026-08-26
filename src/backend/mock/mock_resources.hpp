#pragma once

#include "glcompat/core/backend_resources.hpp"
#include <string>

namespace glcompat {

// Mock backend resource handles. They carry only enough bookkeeping to make
// lifetime and creation observable in tests; no native API is involved.
class MockBuffer : public BackendBuffer {
public:
    int id = 0;
};
class MockTexture : public BackendTexture {
public:
    int id = 0;
};
class MockRenderbuffer : public BackendRenderbuffer {
public:
    int id = 0;
};
class MockFramebuffer : public BackendFramebuffer {
public:
    int id = 0;
};
class MockVertexArray : public BackendVertexArray {
public:
    int id = 0;
};
class MockShader : public BackendShader {
public:
    int id = 0;
};
class MockProgram : public BackendProgram {
public:
    int id = 0;
};

} // namespace glcompat
