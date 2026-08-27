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

// Program pipelines (SPEC §7.4): name management is unconditional; the stateful
// operations are gated by the ProgramPipelines capability.
TEST_CASE("pp_gen_is_delete_pipeline") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName p[2] = {0, 0};
    ctx.genProgramPipelines(2, p);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_NE(p[0], 0u);
    EXPECT_NE(p[1], 0u);
    EXPECT_TRUE(ctx.isProgramPipeline(p[0]));
    EXPECT_TRUE(ctx.isProgramPipeline(p[1]));

    // A program object name must not read as a pipeline, and 0 never is one.
    GLObjectName prog = ctx.createProgram();
    EXPECT_FALSE(ctx.isProgramPipeline(prog));
    EXPECT_FALSE(ctx.isProgramPipeline(0));

    ctx.deleteProgramPipelines(2, p);
    EXPECT_FALSE(ctx.isProgramPipeline(p[0]));
    EXPECT_FALSE(ctx.isProgramPipeline(p[1]));
}

TEST_CASE("pp_create_shader_programv_builds_linked_separable_program") {
    auto backend = makeBackend();
    Context ctx(*backend);

    const char* src = "void main(){}";
    GLObjectName prog = ctx.createShaderProgramv(GL_VERTEX_SHADER, 1, &src);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    EXPECT_NE(prog, 0u);
    EXPECT_EQ(ctx.getProgramiv(prog, GL_LINK_STATUS), 1);
    EXPECT_NE(ctx.getProgram(prog), nullptr);
}

TEST_CASE("pp_use_program_stages_requires_separable_program") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName pipeline = 0;
    ctx.genProgramPipelines(1, &pipeline);

    // A separable program built via glCreateShaderProgramv is accepted.
    const char* src = "void main(){}";
    GLObjectName sep = ctx.createShaderProgramv(GL_VERTEX_SHADER, 1, &src);
    ctx.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, sep);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    // A normal (non-separable) linked program is rejected.
    GLObjectName vs = ctx.createShader(GL_VERTEX_SHADER);
    ctx.shaderSource(vs, "void main(){}");
    ctx.compileShader(vs);
    GLObjectName nonsep = ctx.createProgram();
    ctx.attachShader(nonsep, vs);
    ctx.linkProgram(nonsep);
    EXPECT_EQ(ctx.getProgramiv(nonsep, GL_LINK_STATUS), 1);

    ctx.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, nonsep);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("pp_use_program_stages_maps_stage_and_queries") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName pipeline = 0;
    ctx.genProgramPipelines(1, &pipeline);
    const char* src = "void main(){}";
    GLObjectName sep = ctx.createShaderProgramv(GL_VERTEX_SHADER, 1, &src);

    ctx.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, sep);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t v = -1;
    ctx.getProgramPipelineiv(pipeline, GL_VERTEX_SHADER, &v);
    EXPECT_EQ(static_cast<GLObjectName>(v), sep);

    int32_t f = -1;
    ctx.getProgramPipelineiv(pipeline, GL_FRAGMENT_SHADER, &f);
    EXPECT_EQ(static_cast<GLObjectName>(f), 0u);
}

TEST_CASE("pp_active_shader_program_feeds_all_stages") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName pipeline = 0;
    ctx.genProgramPipelines(1, &pipeline);
    const char* src = "void main(){}";
    GLObjectName sep = ctx.createShaderProgramv(GL_VERTEX_SHADER, 1, &src);

    ctx.activeShaderProgram(pipeline, sep);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    // UseProgramStages(..., 0) references the active program for every stage.
    ctx.useProgramStages(pipeline, GL_ALL_SHADER_BITS, 0);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    int32_t active = -1;
    ctx.getProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &active);
    EXPECT_EQ(static_cast<GLObjectName>(active), sep);

    int32_t v = -1;
    ctx.getProgramPipelineiv(pipeline, GL_VERTEX_SHADER, &v);
    EXPECT_EQ(static_cast<GLObjectName>(v), sep);
}

TEST_CASE("pp_use_program_stages_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName pipeline = 0;
    ctx.genProgramPipelines(1, &pipeline);
    const char* src = "void main(){}";
    GLObjectName sep = ctx.createShaderProgramv(GL_VERTEX_SHADER, 1, &src);

    // Invalid stage bit combination -> INVALID_VALUE.
    ctx.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT | 0x80000000u, sep);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Ungenerated pipeline -> INVALID_OPERATION.
    ctx.useProgramStages(777, GL_VERTEX_SHADER_BIT, sep);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("pp_bind_program_pipeline_forwards_to_backend") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName pipeline = 0;
    ctx.genProgramPipelines(1, &pipeline);

    ctx.bindProgramPipeline(pipeline);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    ctx.flushState();
    EXPECT_EQ(backend->bindProgramPipelineCalls, 1);
    EXPECT_EQ(backend->lastProgramPipeline, pipeline);

    // Binding 0 (unbind) is forwarded too.
    ctx.bindProgramPipeline(0);
    ctx.flushState();
    EXPECT_EQ(backend->bindProgramPipelineCalls, 2);
    EXPECT_EQ(backend->lastProgramPipeline, 0u);

    // Binding an ungenerated name -> INVALID_OPERATION.
    ctx.bindProgramPipeline(555);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("pp_get_program_pipelineiv_validation") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName pipeline = 0;
    ctx.genProgramPipelines(1, &pipeline);

    // 0 / not-a-pipeline -> INVALID_OPERATION.
    int32_t dummy = 0;
    ctx.getProgramPipelineiv(0, GL_ACTIVE_PROGRAM, &dummy);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
    ctx.getProgramPipelineiv(999, GL_ACTIVE_PROGRAM, &dummy);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    // Null params -> INVALID_VALUE.
    ctx.getProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, nullptr);
    EXPECT_EQ(ctx.getError(), GLError::InvalidValue);

    // Unknown pname -> INVALID_ENUM.
    ctx.getProgramPipelineiv(pipeline, 0xDEAD, &dummy);
    EXPECT_EQ(ctx.getError(), GLError::InvalidEnum);
}

TEST_CASE("pp_validate_and_info_log") {
    auto backend = makeBackend();
    Context ctx(*backend);

    GLObjectName pipeline = 0;
    ctx.genProgramPipelines(1, &pipeline);

    int32_t valid = -1;
    ctx.getProgramPipelineiv(pipeline, GL_VALID_STATUS, &valid);
    EXPECT_EQ(valid, 0);

    ctx.validateProgramPipeline(pipeline);
    EXPECT_EQ(ctx.getError(), GLError::NoError);
    ctx.getProgramPipelineiv(pipeline, GL_VALID_STATUS, &valid);
    EXPECT_EQ(valid, 1);

    int32_t len = -1;
    ctx.getProgramPipelineiv(pipeline, GL_INFO_LOG_LENGTH, &len);
    EXPECT_EQ(len, 1); // empty log -> length 1 (nul terminator)

    char log[16] = {0};
    int32_t outLen = -1;
    ctx.getProgramPipelineInfoLog(pipeline, sizeof(log), &outLen, log);
    EXPECT_EQ(outLen, 0);
    EXPECT_EQ(log[0], '\0');
}

TEST_CASE("pp_gated_by_program_pipelines_capability") {
    auto backend = makeBackend();
    backend->setCapability(Feature::ProgramPipelines, FeatureSupport::Unsupported);
    Context ctx(*backend);

    // Name management still works.
    GLObjectName p = 0;
    ctx.genProgramPipelines(1, &p);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    // Stateful operations report INVALID_OPERATION when unsupported.
    ctx.bindProgramPipeline(p);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);

    const char* src = "void main(){}";
    GLObjectName prog = ctx.createShaderProgramv(GL_VERTEX_SHADER, 1, &src);
    EXPECT_EQ(prog, 0u);
    EXPECT_EQ(ctx.getError(), GLError::InvalidOperation);
}

TEST_CASE("pp_gl_api_entry_points") {
    auto backend = makeBackend();
    Context ctx(*backend);
    setCurrentContext(&ctx);

    GLuint pipeline = 0;
    glGenProgramPipelines(1, &pipeline);
    EXPECT_NE(pipeline, 0u);
    EXPECT_TRUE(glIsProgramPipeline(pipeline));

    const char* src = "void main(){}";
    GLuint sep = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &src);
    EXPECT_NE(sep, 0u);

    glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, sep);
    EXPECT_EQ(ctx.getError(), GLError::NoError);

    GLint v = 0;
    glGetProgramPipelineiv(pipeline, GL_VERTEX_SHADER, &v);
    EXPECT_EQ(static_cast<GLuint>(v), sep);

    glBindProgramPipeline(pipeline);
    glFlushState();
    EXPECT_EQ(backend->bindProgramPipelineCalls, 1);

    glDeleteProgramPipelines(1, &pipeline);
    EXPECT_FALSE(glIsProgramPipeline(pipeline));

    setCurrentContext(nullptr);
}
