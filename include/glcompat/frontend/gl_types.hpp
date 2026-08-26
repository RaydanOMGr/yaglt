#pragma once

#include <cstdint>

namespace glcompat {

// Minimal GL scalar typedefs used by the frontend API surface. These mirror the
// desktop OpenGL types so callers can use the familiar signatures.
using GLenum = uint32_t;
using GLuint = uint32_t;
using GLsizei = int32_t;
using GLint = int32_t;
using GLsizeiptr = intptr_t;
using GLintptr = intptr_t;
using GLvoid = void;
using GLchar = char;
using GLboolean = unsigned char;

// Selected OpenGL constants required by the implemented API subset.
// Values match the desktop GL specification so the frontend is compatible.
constexpr GLenum GL_NO_ERROR = 0x0000;
constexpr GLenum GL_INVALID_ENUM = 0x0500;
constexpr GLenum GL_INVALID_VALUE = 0x0501;
constexpr GLenum GL_INVALID_OPERATION = 0x0502;

constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;

constexpr GLenum GL_STATIC_DRAW = 0x88E4;
constexpr GLenum GL_DYNAMIC_DRAW = 0x88E8;
constexpr GLenum GL_STREAM_DRAW = 0x88E0;

constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_RENDERBUFFER = 0x8D41;
constexpr GLenum GL_FRAMEBUFFER = 0x8D40;
constexpr GLenum GL_READ_FRAMEBUFFER = 0x8CA8;
constexpr GLenum GL_DRAW_FRAMEBUFFER = 0x8CA9;
constexpr GLenum GL_VERTEX_ARRAY = 0x8074;

// Indexed buffer binding points (UBO / SSBO / transform feedback).
constexpr GLenum GL_UNIFORM_BUFFER = 0x8A11;
constexpr GLenum GL_TRANSFORM_FEEDBACK_BUFFER = 0x8C8E;
constexpr GLenum GL_SHADER_STORAGE_BUFFER = 0x90D2;

// Booleans.
constexpr GLenum GL_TRUE = 1;
constexpr GLenum GL_FALSE = 0;

// Shader stages.
constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
constexpr GLenum GL_GEOMETRY_SHADER = 0x8DD9;
constexpr GLenum GL_TESS_CONTROL_SHADER = 0x8E88;
constexpr GLenum GL_TESS_EVALUATION_SHADER = 0x8E87;
constexpr GLenum GL_COMPUTE_SHADER = 0x91B9;

// Shader / program query parameters.
constexpr GLenum GL_COMPILE_STATUS = 0x8B81;
constexpr GLenum GL_LINK_STATUS = 0x8B82;

// Vertex attribute types.
constexpr GLenum GL_FLOAT = 0x1406;
constexpr GLenum GL_FLOAT_VEC2 = 0x8B50;
constexpr GLenum GL_FLOAT_VEC3 = 0x8B51;
constexpr GLenum GL_FLOAT_VEC4 = 0x8B52;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;

} // namespace glcompat
