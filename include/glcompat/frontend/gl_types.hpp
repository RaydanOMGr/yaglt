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
using GLdouble = double;
using GLvoid = void;
using GLchar = char;
using GLboolean = unsigned char;
using GLbitfield = uint32_t;
using GLubyte = unsigned char;
using GLuint64 = uint64_t;
using GLint64 = int64_t;

// Opaque sync object handle (SPEC §4 / §20, ARB_sync). The frontend owns the
// SyncObject instance and hands back an opaque pointer; the backend never sees
// the raw pointer (SPEC §3: backend handles never leak into the generic API).
struct __GLsync;
using GLsync = __GLsync*;

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
constexpr GLenum GL_NUM_EXTENSIONS = 0x821D;

constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
constexpr GLenum GL_COPY_READ_BUFFER = 0x8F36;
constexpr GLenum GL_COPY_WRITE_BUFFER = 0x8F37;
constexpr GLenum GL_PIXEL_PACK_BUFFER = 0x88EB;
constexpr GLenum GL_PIXEL_UNPACK_BUFFER = 0x88EC;
constexpr GLenum GL_QUERY_BUFFER = 0x9192;
constexpr GLenum GL_BUFFER_MAP_POINTER = 0x88BD;

// Object label identifier namespaces (SPEC §22.2, glObjectLabel /
// glGetObjectLabel). GL_TEXTURE / GL_RENDERBUFFER / GL_FRAMEBUFFER /
// GL_VERTEX_ARRAY reuse their existing target tokens (defined elsewhere here).
constexpr GLenum GL_BUFFER = 0x82E0;
constexpr GLenum GL_SHADER = 0x82E1;
constexpr GLenum GL_PROGRAM = 0x82E2;
constexpr GLenum GL_QUERY = 0x82E3;
constexpr GLenum GL_PROGRAM_PIPELINE = 0x82E4;
constexpr GLenum GL_SAMPLER = 0x82E6;
constexpr GLenum GL_MAX_LABEL_LENGTH = 0x82E8;
constexpr GLenum GL_TRANSFORM_FEEDBACK = 0x8E22;
// Maximum object-label length accepted by glObjectLabel (SPEC §22.2: an
// implementation must support at least 256 characters).
constexpr int kMaxObjectLabelLength = 256;

constexpr GLenum GL_STATIC_DRAW = 0x88E4;
constexpr GLenum GL_DYNAMIC_DRAW = 0x88E8;
constexpr GLenum GL_STREAM_DRAW = 0x88E0;
constexpr GLenum GL_STATIC_READ = 0x88E5;
constexpr GLenum GL_STATIC_COPY = 0x88E6;
constexpr GLenum GL_DYNAMIC_READ = 0x88E9;
constexpr GLenum GL_DYNAMIC_COPY = 0x88EA;
constexpr GLenum GL_STREAM_READ = 0x88E1;
constexpr GLenum GL_STREAM_COPY = 0x88E2;

// Buffer parameter queries (SPEC §6 / §22, glGetBufferParameteriv).
constexpr GLenum GL_BUFFER_SIZE = 0x8764;
constexpr GLenum GL_BUFFER_USAGE = 0x8765;
constexpr GLenum GL_BUFFER_ACCESS = 0x88BB;
constexpr GLenum GL_BUFFER_ACCESS_FLAGS = 0x911F;
constexpr GLenum GL_BUFFER_IMMUTABLE_STORAGE = 0x821F;
constexpr GLenum GL_BUFFER_MAPPED = 0x88BC;
constexpr GLenum GL_BUFFER_MAP_LENGTH = 0x9120;
constexpr GLenum GL_BUFFER_MAP_OFFSET = 0x9121;

// Buffer mapping access modes (glMapBuffer access, SPEC §6.1).
constexpr GLenum GL_READ_ONLY = 0x88B8;
constexpr GLenum GL_WRITE_ONLY = 0x88B9;
constexpr GLenum GL_READ_WRITE = 0x88BA;

// Buffer mapping access bits (glMapBufferRange access, SPEC §6.1).
constexpr GLenum GL_MAP_READ_BIT = 0x0001;
constexpr GLenum GL_MAP_WRITE_BIT = 0x0002;
constexpr GLenum GL_MAP_INVALIDATE_RANGE_BIT = 0x0004;
constexpr GLenum GL_MAP_INVALIDATE_BUFFER_BIT = 0x0008;
constexpr GLenum GL_MAP_FLUSH_EXPLICIT_BIT = 0x0010;
constexpr GLenum GL_MAP_UNSIGNED_BYTE_BIT = 0x0020;
constexpr GLenum GL_MAP_PERSISTENT_BIT = 0x0040;

constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_RENDERBUFFER = 0x8D41;
constexpr GLenum GL_RENDERBUFFER_WIDTH = 0x8D42;
constexpr GLenum GL_RENDERBUFFER_HEIGHT = 0x8D43;
constexpr GLenum GL_RENDERBUFFER_INTERNAL_FORMAT = 0x8D44;
constexpr GLenum GL_RENDERBUFFER_SAMPLES = 0x8D46;
constexpr GLenum GL_RENDERBUFFER_RED_SIZE = 0x8D50;
constexpr GLenum GL_RENDERBUFFER_GREEN_SIZE = 0x8D51;
constexpr GLenum GL_RENDERBUFFER_BLUE_SIZE = 0x8D52;
constexpr GLenum GL_RENDERBUFFER_ALPHA_SIZE = 0x8D53;
constexpr GLenum GL_RENDERBUFFER_DEPTH_SIZE = 0x8D54;
constexpr GLenum GL_RENDERBUFFER_STENCIL_SIZE = 0x8D55;
constexpr GLenum GL_FRAMEBUFFER = 0x8D40;
constexpr GLenum GL_READ_FRAMEBUFFER = 0x8CA8;
constexpr GLenum GL_DRAW_FRAMEBUFFER = 0x8CA9;
constexpr GLenum GL_VERTEX_ARRAY = 0x8074;

// Internal format queries (SPEC §22.3, glGetInternalformativ / glGetInternalformati64v).
constexpr GLenum GL_NUM_SAMPLE_COUNTS = 0x9380;
constexpr GLenum GL_SAMPLES = 0x80A9;
constexpr GLenum GL_INTERNALFORMAT_SUPPORTED = 0x826F;
constexpr GLenum GL_INTERNALFORMAT_PREFERRED = 0x8270;
constexpr GLenum GL_INTERNALFORMAT_RED_SIZE = 0x8271;
constexpr GLenum GL_INTERNALFORMAT_GREEN_SIZE = 0x8272;
constexpr GLenum GL_INTERNALFORMAT_BLUE_SIZE = 0x8273;
constexpr GLenum GL_INTERNALFORMAT_ALPHA_SIZE = 0x8274;
constexpr GLenum GL_INTERNALFORMAT_DEPTH_SIZE = 0x8275;
constexpr GLenum GL_INTERNALFORMAT_STENCIL_SIZE = 0x8276;
constexpr GLenum GL_INTERNALFORMAT_SHARED_SIZE = 0x8277;
constexpr GLenum GL_INTERNALFORMAT_RED_TYPE = 0x8278;
constexpr GLenum GL_INTERNALFORMAT_GREEN_TYPE = 0x8279;
constexpr GLenum GL_INTERNALFORMAT_BLUE_TYPE = 0x827A;
constexpr GLenum GL_INTERNALFORMAT_ALPHA_TYPE = 0x827B;
constexpr GLenum GL_INTERNALFORMAT_DEPTH_TYPE = 0x827C;
constexpr GLenum GL_INTERNALFORMAT_STENCIL_TYPE = 0x827D;
constexpr GLenum GL_INTERNALFORMAT_COLOR_COMPONENTS = 0x827E;
constexpr GLenum GL_INTERNALFORMAT_COLOR_RENDERABLE = 0x827F;
constexpr GLenum GL_INTERNALFORMAT_DEPTH_RENDERABLE = 0x8280;
constexpr GLenum GL_INTERNALFORMAT_STENCIL_RENDERABLE = 0x8281;
constexpr GLenum GL_INTERNALFORMAT_FRAGMENT_LOAD_STORE = 0x8282;
constexpr GLenum GL_INTERNALFORMAT_VERTEX_ATOMIC = 0x8283;
constexpr GLenum GL_INTERNALFORMAT_FRAGMENT_ATOMIC = 0x8284;
constexpr GLenum GL_INTERNALFORMAT_TEXEL_SIZE = 0x8285;
constexpr GLenum GL_INTERNALFORMAT_TEXTURE_COMPRESSED = 0x8286;
constexpr GLenum GL_INTERNALFORMAT_TEXTURE_COMPRESSED_BLOCK_WIDTH = 0x8287;
constexpr GLenum GL_INTERNALFORMAT_TEXTURE_COMPRESSED_BLOCK_HEIGHT = 0x8288;
constexpr GLenum GL_INTERNALFORMAT_TEXTURE_COMPRESSED_BLOCK_SIZE = 0x8289;
constexpr GLenum GL_INTERNALFORMAT_FRAMEBUFFER_BLEND = 0x828A;
constexpr GLenum GL_INTERNALFORMAT_READ_PIXELS = 0x828B;
constexpr GLenum GL_INTERNALFORMAT_READ_PIXELS_FORMAT = 0x828C;
constexpr GLenum GL_INTERNALFORMAT_READ_PIXELS_TYPE = 0x828D;
constexpr GLenum GL_INTERNALFORMAT_TEXTURE_IMAGE_FORMAT = 0x828E;
constexpr GLenum GL_INTERNALFORMAT_TEXTURE_IMAGE_TYPE = 0x828F;
constexpr GLenum GL_INTERNALFORMAT_GET_TEXTURE_IMAGE_FORMAT = 0x8290;
constexpr GLenum GL_INTERNALFORMAT_GET_TEXTURE_IMAGE_TYPE = 0x8291;
constexpr GLenum GL_INTERNALFORMAT_MANUAL_GENERATE_MIPMAP = 0x8294;
constexpr GLenum GL_INTERNALFORMAT_AUTO_GENERATE_MIPMAP = 0x8295;
constexpr GLenum GL_INTERNALFORMAT_SRGB_READ = 0x8296;
constexpr GLenum GL_INTERNALFORMAT_SRGB_WRITE = 0x8297;
constexpr GLenum GL_INTERNALFORMAT_SRGB_RENDERABLE = 0x8299;

// Texture targets / parameters / formats / filters.
constexpr GLenum GL_TEXTURE_1D = 0x0DE0;
constexpr GLenum GL_TEXTURE_3D = 0x806F;
constexpr GLenum GL_TEXTURE_1D_ARRAY = 0x8C18;
constexpr GLenum GL_TEXTURE_2D_ARRAY = 0x8C1A;
constexpr GLenum GL_TEXTURE_RECTANGLE = 0x84F5;
constexpr GLenum GL_TEXTURE_CUBE_MAP = 0x8513;
constexpr GLenum GL_TEXTURE_CUBE_MAP_POSITIVE_X = 0x8515;
constexpr GLenum GL_TEXTURE_CUBE_MAP_NEGATIVE_X = 0x8516;
constexpr GLenum GL_TEXTURE_CUBE_MAP_POSITIVE_Y = 0x8517;
constexpr GLenum GL_TEXTURE_CUBE_MAP_NEGATIVE_Y = 0x8518;
constexpr GLenum GL_TEXTURE_CUBE_MAP_POSITIVE_Z = 0x8519;
constexpr GLenum GL_TEXTURE_CUBE_MAP_NEGATIVE_Z = 0x851A;
constexpr GLenum GL_TEXTURE_CUBE_MAP_ARRAY = 0x9009;
constexpr GLenum GL_TEXTURE_2D_MULTISAMPLE = 0x9100;
constexpr GLenum GL_TEXTURE_2D_MULTISAMPLE_ARRAY = 0x9102;
constexpr GLenum GL_TEXTURE_BUFFER = 0x8C2A;
constexpr GLenum GL_TEXTURE_BUFFER_DATA_STORE_BINDING = 0x8C2D;

// Texture object parameter / level queries (SPEC §8.1 / §22, DSA getTex*).
constexpr GLenum GL_TEXTURE_WIDTH = 0x1000;
constexpr GLenum GL_TEXTURE_HEIGHT = 0x1001;
constexpr GLenum GL_TEXTURE_DEPTH = 0x8071;
constexpr GLenum GL_TEXTURE_INTERNAL_FORMAT = 0x1003;
constexpr GLenum GL_TEXTURE_RED_TYPE = 0x8C10;
constexpr GLenum GL_TEXTURE_GREEN_TYPE = 0x8C11;
constexpr GLenum GL_TEXTURE_BLUE_TYPE = 0x8C12;
constexpr GLenum GL_TEXTURE_ALPHA_TYPE = 0x8C13;
constexpr GLenum GL_TEXTURE_SAMPLES = 0x9106;
constexpr GLenum GL_TEXTURE_FIXED_SAMPLE_LOCATIONS = 0x9107;
constexpr GLenum GL_TEXTURE_IMMUTABLE_FORMAT = 0x912F;
constexpr GLenum GL_TEXTURE_IMMUTABLE_LEVELS = 0x82DF;
constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
constexpr GLenum GL_TEXTURE_WRAP_S = 0x2802;
constexpr GLenum GL_TEXTURE_WRAP_T = 0x2803;
constexpr GLenum GL_TEXTURE_WRAP_R = 0x8072;
constexpr GLenum GL_NEAREST = 0x2600;
constexpr GLenum GL_LINEAR = 0x2601;
constexpr GLenum GL_NEAREST_MIPMAP_NEAREST = 0x2700;
constexpr GLenum GL_LINEAR_MIPMAP_NEAREST = 0x2701;
constexpr GLenum GL_NEAREST_MIPMAP_LINEAR = 0x2702;
constexpr GLenum GL_LINEAR_MIPMAP_LINEAR = 0x2703;
constexpr GLenum GL_REPEAT = 0x2901;
constexpr GLenum GL_CLAMP_TO_EDGE = 0x812F;
constexpr GLenum GL_CLAMP_TO_BORDER = 0x812D;
constexpr GLenum GL_MIRROR_CLAMP_TO_EDGE = 0x8743;
constexpr GLenum GL_TEXTURE_BORDER_COLOR = 0x1003;
constexpr GLenum GL_TEXTURE_SWIZZLE_RGBA = 0x8E46;
constexpr GLenum GL_MIRRORED_REPEAT = 0x8370;
// Sampler-object LOD / compare parameters (SPEC §8.2).
constexpr GLenum GL_TEXTURE_MIN_LOD = 0x813A;
constexpr GLenum GL_TEXTURE_MAX_LOD = 0x813B;
constexpr GLenum GL_TEXTURE_LOD_BIAS = 0x8501;
constexpr GLenum GL_TEXTURE_COMPARE_MODE = 0x884C;
constexpr GLenum GL_TEXTURE_COMPARE_FUNC = 0x884D;
constexpr GLenum GL_COMPARE_REF_TO_TEXTURE = 0x884E;
// Sampler binding query (SPEC §8.2 / §22).
constexpr GLenum GL_SAMPLER_BINDING = 0x8919;
constexpr GLenum GL_RED = 0x1903;
constexpr GLenum GL_RG = 0x8227;
constexpr GLenum GL_RGB = 0x1907;
constexpr GLenum GL_RGBA = 0x1908;
constexpr GLenum GL_RGBA8 = 0x8058;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;
constexpr GLenum GL_UNSIGNED_SHORT = 0x1403;
constexpr GLenum GL_HALF_FLOAT = 0x140B;
constexpr GLenum GL_TEXTURE0 = 0x84C0;

// Pixel/texel read-back and buffer-clear source format tokens (SPEC §6 / §8.7 /
// table 8.8). These describe the `format` argument of Clear*Buffer*Data.
constexpr GLenum GL_RED_INTEGER = 0x8D94;
constexpr GLenum GL_RG_INTEGER = 0x8228;
constexpr GLenum GL_RGB_INTEGER = 0x8D98;
constexpr GLenum GL_RGBA_INTEGER = 0x8D99;
constexpr GLenum GL_DEPTH_COMPONENT = 0x1902;
constexpr GLenum GL_STENCIL_INDEX = 0x1901;

// Buffer/texel source type tokens (SPEC §8.7 / table 8.7). These describe the
// `type` argument of Clear*Buffer*Data.
constexpr GLenum GL_BYTE = 0x1400;
constexpr GLenum GL_SHORT = 0x1402;
constexpr GLenum GL_INT = 0x1404;
constexpr GLenum GL_UNSIGNED_INT = 0x1405;

// Sized internal formats accepted by Clear*Buffer*Data (SPEC §6 / table 8.24).
// The frontend keeps an authoritative CPU mirror of every buffer's data store,
// so these drive the in-memory fill pattern; the GLES backend (which has no
// native glClearBufferData) is then kept in sync by re-uploading the range via
// glBufferSubData.
constexpr GLenum GL_R8 = 0x8229;
constexpr GLenum GL_RG8 = 0x822B;
constexpr GLenum GL_RGB8 = 0x8051;
constexpr GLenum GL_RGB10_A2 = 0x8059;
constexpr GLenum GL_R11F_G11F_B10F = 0x8C3A;
constexpr GLenum GL_DEPTH24_STENCIL8 = 0x88F0;
constexpr GLenum GL_DEPTH_COMPONENT24 = 0x81A6;
constexpr GLenum GL_DEPTH_COMPONENT32F = 0x8CAC;
constexpr GLenum GL_SRGB8 = 0x8C41;
constexpr GLenum GL_SRGB8_ALPHA8 = 0x8C43;
constexpr GLenum GL_R16F = 0x822D;
constexpr GLenum GL_RG16F = 0x822F;
constexpr GLenum GL_RGB16F = 0x881B;
constexpr GLenum GL_RGBA16F = 0x881A;
constexpr GLenum GL_R32F = 0x822E;
constexpr GLenum GL_RG32F = 0x8230;
constexpr GLenum GL_RGB32F = 0x8815;
constexpr GLenum GL_RGBA32F = 0x8814;
constexpr GLenum GL_R8I = 0x8231;
constexpr GLenum GL_R8UI = 0x8232;
constexpr GLenum GL_R16I = 0x8233;
constexpr GLenum GL_R16UI = 0x8234;
constexpr GLenum GL_R32I = 0x8235;
constexpr GLenum GL_R32UI = 0x8236;
constexpr GLenum GL_RG8I = 0x8237;
constexpr GLenum GL_RG8UI = 0x8238;
constexpr GLenum GL_RG16I = 0x8239;
constexpr GLenum GL_RG16UI = 0x823A;
constexpr GLenum GL_RG32I = 0x823B;
constexpr GLenum GL_RG32UI = 0x823C;
constexpr GLenum GL_RGB8I = 0x8D8F;
constexpr GLenum GL_RGB8UI = 0x8D90;
constexpr GLenum GL_RGB16I = 0x8D89;
constexpr GLenum GL_RGB16UI = 0x8D8A;
constexpr GLenum GL_RGB32I = 0x8D83;
constexpr GLenum GL_RGB32UI = 0x8D84;
constexpr GLenum GL_RGBA8I = 0x8D8E;
constexpr GLenum GL_RGBA8UI = 0x8D7E;
constexpr GLenum GL_RGBA16I = 0x8D88;
constexpr GLenum GL_RGBA16UI = 0x8D76;
constexpr GLenum GL_RGBA32I = 0x8D82;
constexpr GLenum GL_RGBA32UI = 0x8D70;

// Texture-unit state queries (SPEC §2.1 / §10).
constexpr GLenum GL_ACTIVE_TEXTURE = 0x84E0;

// Vertex attribute state queries (SPEC §10.2 / §10.3).
constexpr GLenum GL_CURRENT_VERTEX_ATTRIB = 0x8626;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_ENABLED = 0x8622;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_SIZE = 0x8623;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_STRIDE = 0x8624;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_TYPE = 0x8625;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_NORMALIZED = 0x886A;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_POINTER = 0x8645;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING = 0x889F;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_INTEGER = 0x88FD;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_DIVISOR = 0x88FE;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_LONG = 0x8744;
constexpr GLenum GL_VERTEX_ATTRIB_BINDING = 0x8F4F;
constexpr GLenum GL_VERTEX_ATTRIB_RELATIVE_OFFSET = 0x8F40;
constexpr GLenum GL_ELEMENT_ARRAY_BUFFER_BINDING = 0x8893;

// Hint targets / modes (SPEC §21.1.1, glHint).
constexpr GLenum GL_DONT_CARE = 0x1100;
constexpr GLenum GL_FASTEST = 0x1101;
constexpr GLenum GL_NICEST = 0x1102;
constexpr GLenum GL_PERSPECTIVE_CORRECTION_HINT = 0x0C50;
constexpr GLenum GL_POINT_SMOOTH_HINT = 0x0C51;
constexpr GLenum GL_LINE_SMOOTH_HINT = 0x0C52;
constexpr GLenum GL_POLYGON_SMOOTH_HINT = 0x0C53;
constexpr GLenum GL_FOG_HINT = 0x0C54;
constexpr GLenum GL_TEXTURE_COMPRESSION_HINT = 0x84EF;
constexpr GLenum GL_FRAGMENT_SHADER_DERIVATIVE_HINT = 0x8B8B;
constexpr GLenum GL_GENERATE_MIPMAP_HINT = 0x8192;
constexpr GLenum GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = 0x8B4D;
constexpr GLenum GL_MAX_TEXTURE_IMAGE_UNITS = 0x8872;
constexpr GLenum GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS = 0x8B4C;

// Indexed buffer binding points (UBO / SSBO / transform feedback).
constexpr GLenum GL_UNIFORM_BUFFER = 0x8A11;
constexpr GLenum GL_TRANSFORM_FEEDBACK_BUFFER = 0x8C8E;
constexpr GLenum GL_SHADER_STORAGE_BUFFER = 0x90D2;

// Booleans.
constexpr GLenum GL_TRUE = 1;
constexpr GLenum GL_FALSE = 0;

// Color clamping (glClampColor, SPEC §15.2.3).
constexpr GLenum GL_CLAMP_READ_COLOR = 0x891C;
constexpr GLenum GL_FIXED_ONLY = 0x891D;

// Indirect draw (glDrawArraysIndirect / glDrawElementsIndirect, SPEC §10).
constexpr GLenum GL_DRAW_INDIRECT_BUFFER = 0x8F3F;
constexpr GLenum GL_DRAW_INDIRECT_BUFFER_BINDING = 0x8F43;

// Compute dispatch (SPEC §7.4).
constexpr GLenum GL_DISPATCH_INDIRECT_BUFFER = 0x90EE;
constexpr GLenum GL_DISPATCH_INDIRECT_BUFFER_BINDING = 0x90EF;

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
constexpr GLenum GL_VALIDATE_STATUS = 0x8B83;
constexpr GLenum GL_INFO_LOG_LENGTH = 0x8B84;

// Program pipeline stage bits (glUseProgramStages, SPEC §7.4) and the
// convenience "all stages" mask.
constexpr GLbitfield GL_VERTEX_SHADER_BIT = 0x00000001;
constexpr GLbitfield GL_FRAGMENT_SHADER_BIT = 0x00000002;
constexpr GLbitfield GL_GEOMETRY_SHADER_BIT = 0x00000004;
constexpr GLbitfield GL_TESS_CONTROL_SHADER_BIT = 0x00000008;
constexpr GLbitfield GL_TESS_EVALUATION_SHADER_BIT = 0x00000010;
constexpr GLbitfield GL_COMPUTE_SHADER_BIT = 0x00000020;
constexpr GLbitfield GL_ALL_SHADER_BITS = 0xFFFFFFFFu;

// Program pipeline parameters (glGetProgramPipelineiv, SPEC §7.4).
constexpr GLenum GL_ACTIVE_PROGRAM = 0x8259;
constexpr GLenum GL_PROGRAM_SEPARABLE = 0x8258;
constexpr GLenum GL_PROGRAM_BINARY_RETRIEVABLE_HINT = 0x8257;
// Program binary (SPEC §7.3 / §19.1).
constexpr GLenum GL_PROGRAM_BINARY_LENGTH = 0x8741;
constexpr GLenum GL_NUM_PROGRAM_BINARY_FORMATS = 0x87FE;
constexpr GLenum GL_PROGRAM_BINARY_FORMATS = 0x87FF;
// Shader binary (SPEC §7.2 / ARB_gl_spirv). glShaderBinary consumes SPIR-V.
constexpr GLenum GL_SHADER_BINARY_FORMAT_SPIR_V = 0x9558;
// GL_VALID_STATUS shares the 0x8B83 value with GL_VALIDATE_STATUS (programs).
constexpr GLenum GL_VALID_STATUS = 0x8B83;
constexpr GLenum GL_ATTACHED_SHADERS = 0x8B85;
constexpr GLenum GL_ACTIVE_UNIFORMS = 0x8B86;
constexpr GLenum GL_ACTIVE_UNIFORM_MAX_LENGTH = 0x8B87;
constexpr GLenum GL_ACTIVE_ATTRIBUTES = 0x8B89;
constexpr GLenum GL_ACTIVE_ATTRIBUTE_MAX_LENGTH = 0x8B8A;
constexpr GLenum GL_SHADER_TYPE = 0x8B4F;
constexpr GLenum GL_DELETE_STATUS = 0x8B80;
constexpr GLenum GL_SHADER_SOURCE_LENGTH = 0x8B88;
constexpr GLenum GL_ACTIVE_UNIFORM_BLOCKS = 0x8A36;
constexpr GLenum GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH = 0x8A35;
constexpr GLenum GL_ACTIVE_ATOMIC_COUNTER_BUFFERS = 0x92D9;

// Program-interface reflection (glGetProgramResource*, SPEC §7.3.11).
constexpr GLuint GL_INVALID_INDEX = 0xFFFFFFFFu;
// Valid `programInterface` values.
constexpr GLenum GL_UNIFORM = 0x92E1;
constexpr GLenum GL_UNIFORM_BLOCK = 0x92E2;
constexpr GLenum GL_PROGRAM_INPUT = 0x92E3;
constexpr GLenum GL_PROGRAM_OUTPUT = 0x92E4;
constexpr GLenum GL_BUFFER_VARIABLE = 0x92E5;
constexpr GLenum GL_SHADER_STORAGE_BLOCK = 0x92E6;
constexpr GLenum GL_VERTEX_SUBROUTINE = 0x92E8;
constexpr GLenum GL_TESS_CONTROL_SUBROUTINE = 0x92E9;
constexpr GLenum GL_TESS_EVALUATION_SUBROUTINE = 0x92EA;
constexpr GLenum GL_GEOMETRY_SUBROUTINE = 0x92EB;
constexpr GLenum GL_FRAGMENT_SUBROUTINE = 0x92EC;
constexpr GLenum GL_COMPUTE_SUBROUTINE = 0x92ED;
constexpr GLenum GL_VERTEX_SUBROUTINE_UNIFORM = 0x92EE;
constexpr GLenum GL_TESS_CONTROL_SUBROUTINE_UNIFORM = 0x92EF;
constexpr GLenum GL_TESS_EVALUATION_SUBROUTINE_UNIFORM = 0x92F0;
constexpr GLenum GL_GEOMETRY_SUBROUTINE_UNIFORM = 0x92F1;
constexpr GLenum GL_FRAGMENT_SUBROUTINE_UNIFORM = 0x92F2;
constexpr GLenum GL_COMPUTE_SUBROUTINE_UNIFORM = 0x92F3;
constexpr GLenum GL_TRANSFORM_FEEDBACK_VARYING = 0x92F4;
constexpr GLenum GL_ATOMIC_COUNTER_BUFFER = 0x92C0;
// glGetProgramInterfaceiv pnames (SPEC §7.3.1, used to size reflection).
constexpr GLenum GL_ACTIVE_RESOURCES = 0x92F5;
constexpr GLenum GL_MAX_RESOURCE_NAME_LENGTH = 0x92F6;
constexpr GLenum GL_MAX_NUM_ACTIVE_VARIABLES = 0x92F7;
constexpr GLenum GL_MAX_NUM_COMPATIBLE_SUBROUTINES = 0x92F8;
constexpr GLenum GL_MAX_NAME_LENGTH = 0x92F6;
// Properties accepted by glGetProgramResourceiv.
constexpr GLenum GL_NAME_LENGTH = 0x92F9;
constexpr GLenum GL_TYPE = 0x92FA;
constexpr GLenum GL_ARRAY_SIZE = 0x92FB;
constexpr GLenum GL_OFFSET = 0x92FC;
constexpr GLenum GL_BLOCK_INDEX = 0x92FD;
constexpr GLenum GL_ARRAY_STRIDE = 0x92FE;
constexpr GLenum GL_MATRIX_STRIDE = 0x92FF;
constexpr GLenum GL_IS_ROW_MAJOR = 0x9300;
constexpr GLenum GL_ATOMIC_COUNTER_BUFFER_INDEX = 0x9301;
constexpr GLenum GL_BUFFER_BINDING = 0x9302;
constexpr GLenum GL_BUFFER_DATA_SIZE = 0x9303;
constexpr GLenum GL_NUM_ACTIVE_VARIABLES = 0x9304;
constexpr GLenum GL_ACTIVE_VARIABLES = 0x9305;
constexpr GLenum GL_REFERENCED_BY_VERTEX_SHADER = 0x9306;
constexpr GLenum GL_REFERENCED_BY_TESS_CONTROL_SHADER = 0x9307;
constexpr GLenum GL_REFERENCED_BY_TESS_EVALUATION_SHADER = 0x9308;
constexpr GLenum GL_REFERENCED_BY_GEOMETRY_SHADER = 0x9309;
constexpr GLenum GL_REFERENCED_BY_FRAGMENT_SHADER = 0x930A;
constexpr GLenum GL_REFERENCED_BY_COMPUTE_SHADER = 0x930B;
constexpr GLenum GL_TOP_LEVEL_ARRAY_SIZE = 0x930C;
constexpr GLenum GL_TOP_LEVEL_ARRAY_STRIDE = 0x930D;
constexpr GLenum GL_LOCATION = 0x930E;

// glGetActiveUniformBlockiv pnames (SPEC §7.6 / table 7.7). These differ from the
// program-resource property enums above; the frontend maps each to its property
// equivalent before delegating to GetProgramResourceiv.
constexpr GLenum GL_UNIFORM_BLOCK_BINDING = 0x8A3F;
constexpr GLenum GL_UNIFORM_BLOCK_DATA_SIZE = 0x8A40;
constexpr GLenum GL_UNIFORM_BLOCK_NAME_LENGTH = 0x8A41;
constexpr GLenum GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS = 0x8A42;
constexpr GLenum GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES = 0x8A43;
constexpr GLenum GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER = 0x8A44;
constexpr GLenum GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER = 0x8A45;
constexpr GLenum GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER = 0x8A46;
constexpr GLenum GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER = 0x84F0;
constexpr GLenum GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER = 0x84F1;
constexpr GLenum GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER = 0x90EC;
constexpr GLenum GL_LOCATION_COMPONENT = 0x934A;
constexpr GLenum GL_TRANSFORM_FEEDBACK_BUFFER_INDEX = 0x934B;

// Subroutine reflection (SPEC §7.9). The `*_SUBROUTINE` interface enums above
// name the per-stage interfaces; these are the subroutine query pnames.
constexpr GLenum GL_ACTIVE_SUBROUTINES = 0x8DE5;
constexpr GLenum GL_ACTIVE_SUBROUTINE_UNIFORMS = 0x8DE6;
constexpr GLenum GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS = 0x8E47;
constexpr GLenum GL_ACTIVE_SUBROUTINE_MAX_LENGTH = 0x8E48;
constexpr GLenum GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH = 0x8E49;
constexpr GLenum GL_NUM_COMPATIBLE_SUBROUTINES = 0x8E4A;
constexpr GLenum GL_COMPATIBLE_SUBROUTINES = 0x8E4B;

// Vertex attribute types.
constexpr GLenum GL_FLOAT = 0x1406;
constexpr GLenum GL_FLOAT_VEC2 = 0x8B50;
constexpr GLenum GL_FLOAT_VEC3 = 0x8B51;
constexpr GLenum GL_FLOAT_VEC4 = 0x8B52;

// Framebuffer attachment points and status (SPEC §2.1).
constexpr GLenum GL_NONE = 0;
constexpr GLenum GL_FRONT = 0x0404;
constexpr GLenum GL_BACK = 0x0405;
constexpr GLenum GL_FRONT_AND_BACK = 0x0408;
// Polygon rasterization mode (glPolygonMode, SPEC §11.1).
constexpr GLenum GL_POINT = 0x1B00;
constexpr GLenum GL_LINE = 0x1B01;
constexpr GLenum GL_FILL = 0x1B02;
constexpr GLenum GL_POLYGON_MODE = 0x0B40;
constexpr GLenum GL_LEFT = 0x0406;
constexpr GLenum GL_RIGHT = 0x0407;
constexpr GLenum GL_FRONT_LEFT = 0x0400;
constexpr GLenum GL_FRONT_RIGHT = 0x0401;
constexpr GLenum GL_BACK_LEFT = 0x0402;
constexpr GLenum GL_BACK_RIGHT = 0x0403;
constexpr GLenum GL_COLOR_ATTACHMENT0 = 0x8CE0;
constexpr GLenum GL_COLOR_ATTACHMENT1 = 0x8CE1;
constexpr GLenum GL_COLOR_ATTACHMENT2 = 0x8CE2;
constexpr GLenum GL_COLOR_ATTACHMENT3 = 0x8CE3;
constexpr GLenum GL_COLOR_ATTACHMENT4 = 0x8CE4;
constexpr GLenum GL_COLOR_ATTACHMENT5 = 0x8CE5;
constexpr GLenum GL_COLOR_ATTACHMENT6 = 0x8CE6;
constexpr GLenum GL_COLOR_ATTACHMENT7 = 0x8CE7;
constexpr GLenum GL_COLOR_ATTACHMENT8 = 0x8CE8;
constexpr GLenum GL_COLOR_ATTACHMENT9 = 0x8CE9;
constexpr GLenum GL_COLOR_ATTACHMENT10 = 0x8CEA;
constexpr GLenum GL_COLOR_ATTACHMENT11 = 0x8CEB;
constexpr GLenum GL_COLOR_ATTACHMENT12 = 0x8CEC;
constexpr GLenum GL_COLOR_ATTACHMENT13 = 0x8CED;
constexpr GLenum GL_COLOR_ATTACHMENT14 = 0x8CEE;
constexpr GLenum GL_COLOR_ATTACHMENT15 = 0x8CEF;
constexpr GLenum GL_DEPTH_ATTACHMENT = 0x8D00;
constexpr GLenum GL_STENCIL_ATTACHMENT = 0x8D20;
constexpr GLenum GL_DEPTH_STENCIL_ATTACHMENT = 0x821A;
constexpr GLenum GL_DEPTH_COMPONENT16 = 0x81A5;
constexpr GLenum GL_FRAMEBUFFER_COMPLETE = 0x8CD5;
constexpr GLenum GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT = 0x8CD6;
constexpr GLenum GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7;
constexpr GLenum GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER = 0x8CDB;
constexpr GLenum GL_FRAMEBUFFER_UNSUPPORTED = 0x8CDD;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE = 0x8CD0;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME = 0x8CD1;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL = 0x8CD2;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER = 0x8CD4;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE = 0x8D50;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE = 0x8D51;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE = 0x8D52;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE = 0x8D53;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE = 0x8D54;
constexpr GLenum GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE = 0x8D55;
constexpr GLenum GL_FRAMEBUFFER_DEFAULT_WIDTH = 0x9310;
constexpr GLenum GL_FRAMEBUFFER_DEFAULT_HEIGHT = 0x9311;
constexpr GLenum GL_FRAMEBUFFER_DEFAULT_LAYERS = 0x9312;
constexpr GLenum GL_FRAMEBUFFER_DEFAULT_SAMPLES = 0x9313;
constexpr GLenum GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS = 0x9314;
constexpr GLenum GL_TEXTURE = 0x1702;
constexpr GLenum GL_COLOR = 0x1800;
constexpr GLenum GL_DEPTH = 0x1801;
constexpr GLenum GL_STENCIL = 0x1802;
constexpr GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;

// Clear mask bits (SPEC §2.1, framebuffer clear).
constexpr GLenum GL_DEPTH_BUFFER_BIT = 0x00000100;
constexpr GLenum GL_STENCIL_BUFFER_BIT = 0x00000400;
constexpr GLenum GL_COLOR_BUFFER_BIT = 0x00004000;

// Blend enable (glEnable/glDisable capability, SPEC §17.3).
constexpr GLenum GL_BLEND = 0x0BE2;

// Blend factors (SPEC §17.3.6.2, table 17.2).
constexpr GLenum GL_ZERO = 0x0000;
constexpr GLenum GL_ONE = 0x0001;
constexpr GLenum GL_SRC_COLOR = 0x0300;
constexpr GLenum GL_ONE_MINUS_SRC_COLOR = 0x0301;
constexpr GLenum GL_SRC_ALPHA = 0x0302;
constexpr GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;
constexpr GLenum GL_DST_ALPHA = 0x0304;
constexpr GLenum GL_ONE_MINUS_DST_ALPHA = 0x0305;
constexpr GLenum GL_DST_COLOR = 0x0306;
constexpr GLenum GL_ONE_MINUS_DST_COLOR = 0x0307;
constexpr GLenum GL_SRC_ALPHA_SATURATE = 0x0308;
constexpr GLenum GL_CONSTANT_COLOR = 0x8001;
constexpr GLenum GL_ONE_MINUS_CONSTANT_COLOR = 0x8002;
constexpr GLenum GL_CONSTANT_ALPHA = 0x8003;
constexpr GLenum GL_ONE_MINUS_CONSTANT_ALPHA = 0x8004;

// Blend equations (SPEC §17.3.6.1, table 17.1).
constexpr GLenum GL_FUNC_ADD = 0x8006;
constexpr GLenum GL_MIN = 0x8007;
constexpr GLenum GL_MAX = 0x8008;
constexpr GLenum GL_FUNC_SUBTRACT = 0x800A;
constexpr GLenum GL_FUNC_REVERSE_SUBTRACT = 0x800B;

// Stencil test enable (glEnable/glDisable capability, SPEC §17.3.3).
constexpr GLenum GL_STENCIL_TEST = 0x0B90;

// Stencil comparison functions (SPEC §17.3.3, StencilFunc func).
constexpr GLenum GL_NEVER = 0x0200;
constexpr GLenum GL_LESS = 0x0201;
constexpr GLenum GL_EQUAL = 0x0202;
constexpr GLenum GL_LEQUAL = 0x0203;
constexpr GLenum GL_GREATER = 0x0204;
constexpr GLenum GL_NOTEQUAL = 0x0205;
constexpr GLenum GL_GEQUAL = 0x0206;
constexpr GLenum GL_ALWAYS = 0x0207;

// Stencil operations (SPEC §17.3.3, StencilOp sfail/dpfail/dppass).
constexpr GLenum GL_KEEP = 0x1E00;
constexpr GLenum GL_REPLACE = 0x1E01;
constexpr GLenum GL_INCR = 0x1E02;
constexpr GLenum GL_DECR = 0x1E03;
constexpr GLenum GL_INVERT = 0x150A;
constexpr GLenum GL_INCR_WRAP = 0x8507;
constexpr GLenum GL_DECR_WRAP = 0x8508;

// Rasterization state (SPEC §17.3 / §11 / §14).
constexpr GLenum GL_CULL_FACE = 0x0B44;
constexpr GLenum GL_DEPTH_TEST = 0x0B71;
constexpr GLenum GL_DITHER = 0x0BD0;
constexpr GLenum GL_FRAMEBUFFER_SRGB = 0x0BDA;       // SPEC §15.1.1 (sRGB drawbuffer decoding)
constexpr GLenum GL_SAMPLE_ALPHA_TO_COVERAGE = 0x809E; // SPEC §15.3.1 (alpha-to-coverage)
constexpr GLenum GL_POLYGON_OFFSET_FILL = 0x8037;
constexpr GLenum GL_POLYGON_OFFSET_POINT = 0x2A01;
constexpr GLenum GL_POLYGON_OFFSET_LINE = 0x2A02;
constexpr GLenum GL_CULL_FACE_MODE = 0x0B45;
constexpr GLenum GL_FRONT_FACE = 0x0B46;
constexpr GLenum GL_CW = 0x0900;
constexpr GLenum GL_CCW = 0x0901;
// Provoking vertex convention (SPEC §11, glProvokingVertex / glGet GL_PROVOKING_VERTEX).
constexpr GLenum GL_FIRST_VERTEX_CONVENTION = 0x8E4D;
constexpr GLenum GL_LAST_VERTEX_CONVENTION = 0x8E4E;
constexpr GLenum GL_PROVOKING_VERTEX = 0x8E4F;
constexpr GLenum GL_POINT_SIZE = 0x0B11;
constexpr GLenum GL_LINE_WIDTH = 0x0B21;
constexpr GLenum GL_POLYGON_OFFSET_FACTOR = 0x8038;
constexpr GLenum GL_POLYGON_OFFSET_UNITS = 0x2A00;

// Depth func already has GL_LESS etc. above; add writemask + range.
constexpr GLenum GL_DEPTH_WRITEMASK = 0x0B72;
constexpr GLenum GL_COLOR_WRITEMASK = 0x0C23;
constexpr GLenum GL_SAMPLE_COVERAGE_VALUE = 0x80B9;
constexpr GLenum GL_SAMPLE_COVERAGE_INVERT = 0x80AB;
// Multisample raster mask / minimum sample shading (SPEC §11.5).
constexpr GLenum GL_SAMPLE_MASK = 0x8E51;
constexpr GLenum GL_MIN_SAMPLE_SHADING = 0x8C36;
// Per-sample location query (SPEC §14.3.1 glGetMultisamplefv).
constexpr GLenum GL_SAMPLE_POSITION = 0x8E50;
constexpr GLenum GL_DEPTH_RANGE = 0x0B70;
constexpr GLenum GL_DEPTH_FUNC = 0x0B74;

// Viewport / scissor box queries (SPEC §22).
constexpr GLenum GL_VIEWPORT = 0x0BA2;
constexpr GLenum GL_SCISSOR_BOX = 0x0C10;
constexpr GLenum GL_SCISSOR_TEST = 0x0C11;

// Clear value queries (SPEC §22).
constexpr GLenum GL_COLOR_CLEAR_VALUE = 0x0C22;
constexpr GLenum GL_DEPTH_CLEAR_VALUE = 0x0B73;

// Active program query (SPEC §7.14).
constexpr GLenum GL_CURRENT_PROGRAM = 0x8B8D;

// Blend query pnames (SPEC §17.3.6).
constexpr GLenum GL_BLEND_SRC_RGB = 0x80C9;
constexpr GLenum GL_BLEND_DST_RGB = 0x80CA;
constexpr GLenum GL_BLEND_SRC_ALPHA = 0x80CB;
constexpr GLenum GL_BLEND_DST_ALPHA = 0x80CC;
constexpr GLenum GL_BLEND_EQUATION_RGB = 0x8009;
constexpr GLenum GL_BLEND_EQUATION_ALPHA = 0x883D;
constexpr GLenum GL_BLEND_COLOR = 0x8005;

// Query object targets (SPEC §4 / §19).
constexpr GLenum GL_SAMPLES_PASSED = 0x8914;
constexpr GLenum GL_ANY_SAMPLES_PASSED = 0x8C2F;
constexpr GLenum GL_ANY_SAMPLES_PASSED_CONSERVATIVE = 0x8D6A;
constexpr GLenum GL_PRIMITIVES_GENERATED = 0x8C87;
constexpr GLenum GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN = 0x8C88;
constexpr GLenum GL_TIME_ELAPSED = 0x88BF;
constexpr GLenum GL_TIMESTAMP = 0x8E28;

// Query parameter names (SPEC §4 / §19, glGetQueryiv / glGetQueryObject*).
constexpr GLenum GL_QUERY_COUNTER_BITS = 0x8864;
constexpr GLenum GL_CURRENT_QUERY = 0x8865;
constexpr GLenum GL_QUERY_RESULT = 0x8866;
constexpr GLenum GL_QUERY_RESULT_AVAILABLE = 0x8867;

// Sync object parameters / status (SPEC §4 / §20, ARB_sync).
constexpr GLenum GL_SYNC_STATUS = 0x9114;
constexpr GLenum GL_SIGNALED = 0x9119;
constexpr GLenum GL_UNSIGNALED = 0x9118;
constexpr GLenum GL_SYNC_CONDITION = 0x9113;
constexpr GLenum GL_SYNC_GPU_COMMANDS_COMPLETE = 0x9117;
constexpr GLenum GL_SYNC_FLAGS = 0x9115;
constexpr GLenum GL_ALREADY_SIGNALED = 0x911A;
constexpr GLenum GL_TIMEOUT_EXPIRED = 0x911B;
constexpr GLenum GL_CONDITION_SATISFIED = 0x911C;
constexpr GLenum GL_WAIT_FAILED = 0x911D;
constexpr GLenum GL_SYNC_FLUSH_COMMANDS_BIT = 0x00000001;

// Color logic op (SPEC §17.3.4, glLogicOp). Enabled via GL_COLOR_LOGIC_OP.
constexpr GLenum GL_COLOR_LOGIC_OP = 0x0BF2;
constexpr GLenum GL_LOGIC_OP_MODE = 0x0AFC;
constexpr GLenum GL_CLEAR = 0x1500;
constexpr GLenum GL_AND = 0x1501;
constexpr GLenum GL_AND_REVERSE = 0x1502;
constexpr GLenum GL_COPY = 0x1503;
constexpr GLenum GL_AND_INVERTED = 0x1504;
constexpr GLenum GL_NOOP = 0x1505;
constexpr GLenum GL_XOR = 0x1506;
constexpr GLenum GL_OR = 0x1507;
constexpr GLenum GL_NOR = 0x1508;
constexpr GLenum GL_EQUIV = 0x1509;
// GL_INVERT (0x150A) is defined in the stencil-op block above; it is shared by
// glStencilOp and glLogicOp, so it is intentionally declared only once.
constexpr GLenum GL_OR_REVERSE = 0x150B;
constexpr GLenum GL_COPY_INVERTED = 0x150C;
constexpr GLenum GL_OR_INVERTED = 0x150D;
constexpr GLenum GL_NAND = 0x150E;
constexpr GLenum GL_SET = 0x150F;

// Primitive restart (SPEC §10.4, glPrimitiveRestartIndex + GL_PRIMITIVE_RESTART).
constexpr GLenum GL_PRIMITIVE_RESTART = 0x8F9D;
constexpr GLenum GL_PRIMITIVE_RESTART_FIXED_INDEX = 0x8FDE;
constexpr GLenum GL_PRIMITIVE_RESTART_INDEX = 0x8F9E;

} // namespace glcompat
