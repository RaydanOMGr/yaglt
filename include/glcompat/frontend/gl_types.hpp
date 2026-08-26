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

} // namespace glcompat
