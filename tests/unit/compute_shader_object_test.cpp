#include "test_framework.hpp"

#include "glcompat/frontend/context.hpp"
#include "glcompat/frontend/gl_api.hpp"
#include "glcompat/frontend/gl_types.hpp"
#include "src/backend/mock/mock_backend.hpp"

using namespace glcompat;

static std::unique_ptr<MockBackend> makeBackend() {
    auto b = std::make_unique<MockBackend>();
    b->initialize();
    return b;
}

// A complete compute program (SPEC §7.1/§7.4): create the compute shader object,
// source + compile, attach to a program, link, make active, and dispatch. The
// mock records the dispatch without a driver; the frontend validates the feature.
// Compute is native in the GLES 3.1 mock baseline.
TEST_CASE("compute_program_full_object_lifecycle") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName cs = ctx.createShader(GL_COMPUTE_SHADER);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_NE(cs, 0u);

    ctx.shaderSource(cs,
        "#version 310 es\n"
        "layout(local_size_x = 1) in;\n"
        "void main() {}\n");
    ctx.compileShader(cs);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getShaderiv(cs, GL_COMPILE_STATUS), 1);
    EXPECT_EQ(ctx.getShader(cs)->compiled, true);

    GLObjectName prog = ctx.createProgram();
    ctx.attachShader(prog, cs);
    ctx.linkProgram(prog);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(ctx.getProgramiv(prog, GL_LINK_STATUS), 1);
    EXPECT_EQ(ctx.getProgram(prog)->linked, true);
    EXPECT_EQ(ctx.getProgramiv(prog, GL_ATTACHED_SHADERS), 1);

    ctx.state().useProgram(prog);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    ctx.dispatchCompute(4, 2, 1);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_EQ(backend->dispatchComputeCalls, 1);
    EXPECT_EQ(backend->lastDispatchX, 4u);
    EXPECT_EQ(backend->lastDispatchY, 2u);
    EXPECT_EQ(backend->lastDispatchZ, 1u);
}

// The compute stage is native in the GLES 3.1 mock baseline. Geometry/tessellation
// remain honestly Unsupported (no GLES equivalent) and are rejected at creation.
TEST_CASE("geometry_stage_still_rejected") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName gs = ctx.createShader(GL_GEOMETRY_SHADER);
    EXPECT_EQ(gs, 0u);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}
