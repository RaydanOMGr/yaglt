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
using GLfloat = float;
using GLvoid = void;
using GLchar = char;
using GLboolean = unsigned char;
using GLubyte = unsigned char;

// Selected OpenGL constants required by the implemented API subset.
// Values match the desktop GL specification so the frontend is compatible.
constexpr GLenum GL_NO_ERROR = 0x0000;
constexpr GLenum GL_INVALID_ENUM = 0x0500;
constexpr GLenum GL_INVALID_VALUE = 0x0501;
constexpr GLenum GL_INVALID_OPERATION = 0x0502;

// GetString query names (SPEC §22.2).
constexpr GLenum GL_VENDOR = 0x1F00;
constexpr GLenum GL_RENDERER = 0x1F01;
constexpr GLenum GL_VERSION = 0x1F02;
constexpr GLenum GL_EXTENSIONS = 0x1F03;
constexpr GLenum GL_SHADING_LANGUAGE_VERSION = 0x8B8C;

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

// Texture targets / parameters / formats / filters.
constexpr GLenum GL_TEXTURE_1D = 0x0DE0;
constexpr GLenum GL_TEXTURE_3D = 0x806F;
constexpr GLenum GL_TEXTURE_CUBE_MAP = 0x8513;
constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
constexpr GLenum GL_TEXTURE_WRAP_S = 0x2802;
constexpr GLenum GL_TEXTURE_WRAP_T = 0x2803;
constexpr GLenum GL_NEAREST = 0x2600;
constexpr GLenum GL_LINEAR = 0x2601;
constexpr GLenum GL_NEAREST_MIPMAP_NEAREST = 0x2700;
constexpr GLenum GL_LINEAR_MIPMAP_LINEAR = 0x2703;
constexpr GLenum GL_REPEAT = 0x2901;
constexpr GLenum GL_CLAMP_TO_EDGE = 0x812F;
constexpr GLenum GL_MIRRORED_REPEAT = 0x8370;
constexpr GLenum GL_RED = 0x1903;
constexpr GLenum GL_RG = 0x8227;
constexpr GLenum GL_RGB = 0x1907;
constexpr GLenum GL_RGBA = 0x1908;
constexpr GLenum GL_RGBA8 = 0x8058;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;
constexpr GLenum GL_UNSIGNED_SHORT = 0x1403;
constexpr GLenum GL_HALF_FLOAT = 0x140B;
constexpr GLenum GL_TEXTURE0 = 0x84C0;

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

// Framebuffer attachment points and status (SPEC §2.1).
constexpr GLenum GL_COLOR_ATTACHMENT0 = 0x8CE0;
constexpr GLenum GL_DEPTH_ATTACHMENT = 0x8D00;
constexpr GLenum GL_STENCIL_ATTACHMENT = 0x8D20;
constexpr GLenum GL_DEPTH_STENCIL_ATTACHMENT = 0x821A;
constexpr GLenum GL_DEPTH_COMPONENT16 = 0x81A5;
constexpr GLenum GL_FRAMEBUFFER_COMPLETE = 0x8CD5;
constexpr GLenum GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT = 0x8CD6;
constexpr GLenum GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7;
constexpr GLenum GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER = 0x8CDB;
constexpr GLenum GL_FRAMEBUFFER_UNSUPPORTED = 0x8CDD;
constexpr GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;

} // namespace glcompat
