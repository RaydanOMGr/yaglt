\# 0. Project Identity



The project name is:



\*\*YAGLT — Yet Another GL Translator\*\*



Use `YAGLT` as the project's canonical name and `yaglt` where a lowercase identifier is required.



Examples:



```text

Project: YAGLT

Full name: Yet Another GL Translator

Repository: yaglt

Library identifier: yaglt

```



The name is intentionally generic and self-aware. Do not rename the project to something more elaborate unless explicitly instructed to do so.



YAGLT is intended to be a backend-agnostic OpenGL compatibility and translation layer.



The name must not imply that OpenGL ES is the only supported backend.



The intended architecture is:



```text

&#x20;                        YAGLT

&#x20;                          │

&#x20;                OpenGL 4.6 Frontend

&#x20;                          │

&#x20;                 Backend Abstraction

&#x20;                          │

&#x20;         ┌────────────────┼────────────────┐

&#x20;         ▼                ▼                ▼

&#x20;        GLES           Vulkan        Future Backend

```



The project's primary initial target is OpenGL 4.6-compatible behavior translated to OpenGL ES, particularly on Android, but the architecture must remain suitable for other graphics APIs.



Use the name consistently in:



\* README files.

\* documentation.

\* build configuration.

\* library/package names.

\* logging where appropriate.

\* test infrastructure.

\* generated artifacts.

\* Git repository metadata where appropriate.



The README should introduce the project approximately as:



> \*\*YAGLT — Yet Another GL Translator\*\*

> A backend-agnostic OpenGL 4.6 compatibility and translation layer.



Do not use the project name as justification for poor engineering. "Yet Another" is a name, not an excuse to create yet another giant unmaintainable pile of graphics code.



\# OpenGL 4.6 Compatibility Translation Layer — Agent Implementation Specification



\## 1. Mission



Develop a production-quality C++ graphics compatibility and translation layer that exposes a \*\*desktop OpenGL 4.6-compatible API\*\*, including the OpenGL compatibility profile where practical and required by the compatibility target, while translating operations to one or more backend graphics APIs.



The initial primary target is:



\* Android

\* Minimum Android SDK: 21

\* OpenGL ES as the initial rendering backend



The architecture must not be tied to OpenGL ES.



The system must be designed so that additional backends can be implemented without redesigning the frontend, including but not limited to:



\* OpenGL ES

\* Vulkan

\* desktop OpenGL

\* Metal

\* software or test backends

\* future graphics APIs



The project must also support a headless Linux development and testing environment.



The goal is \*\*not feature reduction\*\*.



When a target API or platform lacks a feature, the implementation should prefer:



1\. Native backend implementation.

2\. Extension-based implementation.

3\. Backend-level emulation.

4\. Shader-based emulation.

5\. CPU-side emulation where reasonable.

6\. Explicit unsupported behavior only when implementation is technically impractical or fundamentally impossible.



The project must not silently pretend that unsupported functionality works.



\---



\# 2. Core Design Principles



\## 2.1 Frontend/backend separation



The OpenGL-facing implementation must not directly depend on OpenGL ES.



The architecture should resemble:



```text

Application

&#x20;   │

&#x20;   ▼

OpenGL 4.6 Compatibility API

&#x20;   │

&#x20;   ▼

Frontend State + Object Management

&#x20;   │

&#x20;   ▼

Graphics Abstraction Layer

&#x20;   │

&#x20;   ├── GLES Backend

&#x20;   ├── Vulkan Backend

&#x20;   ├── Test/Mock Backend

&#x20;   └── Future Backends

```



The OpenGL frontend is responsible for:



\* OpenGL API semantics.

\* OpenGL state tracking.

\* Object lifetime.

\* Error behavior.

\* validation.

\* compatibility behavior.

\* feature emulation coordination.

\* OpenGL-visible capabilities.



Backends are responsible for:



\* Native resource creation.

\* command submission.

\* synchronization.

\* shader execution or translation integration.

\* backend-specific resource management.

\* backend-specific feature implementation.



The OpenGL frontend must not contain scattered checks such as:



```cpp

if (androidSdkVersion >= 26) {

&#x20;   ...

}

```



or:



```cpp

if (glesVersion >= 31) {

&#x20;   ...

}

```



throughout unrelated systems.



Version-specific and platform-specific decisions must be centralized behind abstractions.



\---



\# 3. Architecture



Use an object-oriented abstraction model.



Avoid excessive inheritance where composition is clearer, but backend implementations must be replaceable through stable interfaces.



A conceptual structure:



```text

IGraphicsBackend

├── GLESBackend

├── VulkanBackend

├── MockBackend

└── FutureBackend



IResourceFactory

├── GLESResourceFactory

├── VulkanResourceFactory

└── MockResourceFactory



IShaderCompiler

├── ShadercCompiler

├── BackendShaderCompiler

└── TestShaderCompiler



IPlatformCapabilities

├── AndroidCapabilities

├── LinuxCapabilities

└── FuturePlatformCapabilities

```



Exact interface names may differ, but the architecture must preserve these responsibilities.



Backend-specific types must not leak into the generic frontend API.



For example, this is undesirable:



```cpp

class Texture {

&#x20;   GLuint texture;

};

```



Prefer something conceptually similar to:



```cpp

class BackendTexture;



class TextureObject {

private:

&#x20;   std::unique\_ptr<BackendTexture> backendTexture;

};

```



The frontend should interact with backend abstractions rather than backend-native handles whenever possible.



\---



\# 4. Capability System



Implement a centralized capability system.



Capabilities must describe:



\* backend API version.

\* supported extensions.

\* native features.

\* emulated features.

\* unsupported features.

\* platform capabilities.



Example conceptual model:



```cpp

enum class FeatureSupport {

&#x20;   Native,

&#x20;   Emulated,

&#x20;   Unsupported

};

```



The system should allow querying:



```cpp

FeatureSupport getFeatureSupport(Feature feature);

```



or an equivalent design.



The frontend should generally ask the capability system rather than checking:



\* Android API level.

\* GLES version.

\* extension strings.



directly.



The backend or platform initialization phase should perform version and extension detection once.



For example:



```text

Android Platform

&#x20;      │

&#x20;      ▼

Platform Capability Detection

&#x20;      │

&#x20;      ├── API >= 26

&#x20;      │       └── Shared memory implementation available

&#x20;      │

&#x20;      └── API < 26

&#x20;              └── Fallback implementation

```



The rest of the project should interact with a shared-memory abstraction without caring which implementation is active.



Only functionality actually required by the project should be included in abstractions. Do not create giant platform wrappers containing dozens of unused functions.



\---



\# 5. Android Support



Minimum supported Android SDK:



```text

SDK 21

```



The project must attempt to maintain support down to SDK 21.



Do not reduce functionality simply because older Android versions lack a convenient API.



Instead:



\* isolate newer APIs behind platform abstractions.

\* compile conditionally when required.

\* use runtime checks where necessary.

\* provide fallback implementations.



The preferred pattern is:



```text

Generic subsystem

&#x20;       │

&#x20;       ▼

Small platform abstraction

&#x20;       │

&#x20;       ├── Android API >= required version

&#x20;       │       └── Modern implementation

&#x20;       │

&#x20;       └── Older Android

&#x20;               └── Fallback implementation

```



Do not scatter Android API checks throughout rendering code.



For example, if shared memory functionality is required:



```text

SharedMemory abstraction

&#x20;   │

&#x20;   ├── API 26+ implementation

&#x20;   └── API 21-25 fallback

```



The renderer should only interact with the abstraction.



Where possible:



\* compile code conditionally using preprocessor guards.

\* isolate references to unavailable APIs.

\* avoid runtime linkage failures on older Android versions.



Runtime checks may be used where required, but version-specific branching should occur inside centralized platform implementations.



\---



\# 6. Headless Linux Development Environment



The project must support headless Linux execution for development and testing.



This environment should allow:



\* unit tests.

\* API validation.

\* state tracking tests.

\* shader translation tests.

\* resource lifetime tests.

\* integration tests where possible.

\* backend tests.

\* mock rendering.

\* real headless rendering where available.



Possible implementations may include:



\* EGL headless contexts.

\* surfaceless contexts.

\* software rendering.

\* Mesa.

\* llvmpipe.

\* mock backends.



The exact implementation is flexible.



The goal is that core development does not require an Android device.



The test environment should make it possible to run automated tests such as:



```text

git commit

&#x20;   │

&#x20;   ▼

build

&#x20;   │

&#x20;   ▼

unit tests

&#x20;   │

&#x20;   ▼

integration tests

&#x20;   │

&#x20;   ▼

OpenGL compatibility tests

```



The project must remain testable in a standard headless Linux CI-style environment.



\---



\# 7. Shader System



Shader translation is a first-class subsystem.



External libraries may be used, including:



\* shaderc

\* SPIRV-Tools

\* glslang

\* SPIRV-Cross

\* other suitable shader transformation libraries



Do not implement a complete GLSL compiler from scratch unless there is a compelling reason.



The shader pipeline should conceptually support:



```text

OpenGL GLSL

&#x20;   │

&#x20;   ▼

Parse / Compile

&#x20;   │

&#x20;   ▼

Intermediate Representation

&#x20;   │

&#x20;   ├── Validation

&#x20;   ├── Transformation

&#x20;   ├── Feature Emulation

&#x20;   └── Backend Adaptation

&#x20;   │

&#x20;   ▼

Backend Shader

```



Shader translation must account for differences between desktop OpenGL and backend shader languages.



Potential concerns include:



\* GLSL versions.

\* GLSL ES restrictions.

\* precision qualifiers.

\* unsupported shader stages.

\* layout semantics.

\* texture access.

\* image operations.

\* built-in variables.

\* clip-space differences.

\* coordinate conventions.

\* depth range.

\* compatibility-profile functionality.

\* extension features.



Shader feature emulation should occur in the shader pipeline when that is the most appropriate implementation.



Do not duplicate shader transformation logic unnecessarily across backends.



\---



\# 8. OpenGL 4.6 Compatibility



The compatibility target is OpenGL 4.6.



The agent must treat OpenGL compatibility as an explicit engineering objective rather than simply exposing whichever features GLES happens to support.



The implementation should maintain a feature matrix containing:



```text

Feature

Required OpenGL behavior

Backend support

Native / Extension / Emulated / Unsupported

Implementation location

Tests

Known limitations

```



The exact format may be Markdown, JSON, YAML, or another version-controlled format.



Each major OpenGL feature implemented must have:



\* implementation.

\* tests.

\* capability classification.

\* documented backend behavior.



The project must not claim support for functionality that is silently missing.



\---



\# 9. Extensions



The agent must evaluate widely used OpenGL extensions and determine whether they should be supported.



The agent should prioritize extensions based on:



1\. Usage in real applications.

2\. Importance for compatibility.

3\. Feasibility of emulation.

4\. Availability in target backends.

5\. Implementation complexity.



For a supported extension:



```text

Native backend support

&#x20;       ↓

Use native implementation

```



Otherwise:



```text

Backend extension available

&#x20;       ↓

Use extension

```



Otherwise:



```text

Reasonable emulation possible

&#x20;       ↓

Implement emulation

```



Otherwise:



```text

Explicitly report unsupported

```



Extension decisions must be documented.



Do not attempt to implement every obscure extension merely because it exists.



Prioritize widely deployed and practically useful functionality.



\---



\# 10. State Management



OpenGL is heavily stateful.



Implement centralized state tracking.



The backend should not be forced to query driver state repeatedly when the frontend already knows the current state.



State tracking should cover relevant categories including:



\* buffers.

\* textures.

\* framebuffers.

\* vertex arrays.

\* shader programs.

\* pipeline state.

\* blend state.

\* depth state.

\* stencil state.

\* rasterization state.

\* pixel store state.

\* bindings.

\* synchronization.



Avoid redundant backend calls where state has not changed.



However, optimization must never compromise OpenGL-visible behavior.



State tracking must be thoroughly tested.



\---



\# 11. Resource and Object Lifetime



OpenGL object semantics must be represented independently from backend object lifetimes.



Examples:



```text

OpenGL texture object

&#x20;       │

&#x20;       ├── OpenGL-visible name

&#x20;       ├── OpenGL state

&#x20;       └── Backend resource

```



Backend resources may need to be recreated, substituted, lazily allocated, or emulated.



Therefore OpenGL object identity must not simply equal a backend-native handle.



Object lifetime handling must account for:



\* delayed deletion.

\* currently bound objects.

\* shared contexts if supported.

\* synchronization.

\* backend resource recreation.

\* lazy allocation.



Use RAII internally where appropriate.



Avoid manual ownership ambiguity.



\---



\# 12. Feature Emulation



Feature emulation should be modular.



Avoid:



```cpp

if (!supportsFeatureX) {

&#x20;   // 400 lines of random emulation here

}

```



Prefer:



```text

Feature interface

&#x20;   │

&#x20;   ├── Native implementation

&#x20;   ├── Extension implementation

&#x20;   └── Emulation implementation

```



The capability system chooses the appropriate implementation during initialization or feature setup.



Emulation may occur:



\* CPU-side.

\* GPU-side.

\* in shaders.

\* through resource transformations.

\* through command translation.



Emulation implementations must be independently testable.



\---



\# 13. Build System



Use CMake unless there is a compelling technical reason to use something else.



The build system should support:



\* Android.

\* Linux.

\* Debug.

\* Release.

\* sanitizers where supported.

\* unit tests.

\* integration tests.

\* optional backends.

\* optional external dependencies.



Dependencies should preferably be managed through:



\* CMake FetchContent.

\* package managers where appropriate.

\* pinned versions.

\* reproducible configuration.



Do not require the Android environment to build Linux tests.



Do not require Linux-only dependencies for Android builds.



\---



\# 14. Testing Requirements



Testing is mandatory.



Every significant feature requires tests.



The test hierarchy should include:



\## Unit Tests



Examples:



\* state tracking.

\* capability resolution.

\* version fallback logic.

\* object lifetime.

\* error generation.

\* extension detection.

\* shader transformations.



\## Backend Tests



Examples:



\* resource creation.

\* texture translation.

\* framebuffer translation.

\* synchronization.

\* fallback implementations.



\## Integration Tests



Examples:



```text

OpenGL API call sequence

&#x20;       ↓

Frontend

&#x20;       ↓

Backend abstraction

&#x20;       ↓

Mock or GLES backend

&#x20;       ↓

Expected result

```



\## Compatibility Tests



Where possible, compare behavior against expected OpenGL semantics.



The test suite should include:



\* valid API usage.

\* invalid API usage.

\* edge cases.

\* state transitions.

\* resource deletion.

\* extension behavior.

\* fallback paths.

\* older Android API paths.



Tests must specifically exercise fallback implementations.



A fallback that only compiles but is never executed by tests is not considered sufficiently tested.



\---



\# 15. Git Management



The project must use Git actively and continuously.



Before beginning:



1\. Initialize or inspect the repository.

2\. Configure a local Git username.

3\. Configure a local Git email.

4\. Do not modify global user Git configuration unless explicitly instructed.



Use a project-local identity such as:



```text

AI Graphics Agent

graphics-agent@local

```



or another clearly local development identity.



The agent must commit regularly.



Preferred workflow:



```text

Inspect task

&#x20;   ↓

Plan small change

&#x20;   ↓

Implement

&#x20;   ↓

Build

&#x20;   ↓

Run relevant tests

&#x20;   ↓

Commit

```



Commits should be small and meaningful.



Good examples:



```text

feat(gles): add backend texture abstraction

test(state): cover framebuffer binding transitions

fix(android): add API 21 shared-memory fallback

refactor(shader): isolate GLSL transformation stage

```



Bad examples:



```text

stuff

changes

wip

update everything

final fix please

```



The agent must not accumulate thousands of unrelated changes before committing.



When a change introduces regressions:



1\. Identify the commit that introduced the regression.

2\. Inspect the diff.

3\. Attempt a focused fix.

4\. If the change is fundamentally incorrect, revert or reset to the previous known-good state.

5\. Preserve a useful Git history.



The Git history is part of the engineering process.



Use:



\* `git log`

\* `git diff`

\* `git status`

\* `git blame`



when useful for understanding regressions and implementation history.



Do not rewrite history unnecessarily.



Do not force-push.



Do not destroy a known-good implementation simply to pursue an experimental refactor.



\---



\# 16. Continuous Validation



Before every commit, run the smallest relevant validation set.



Before major milestones, run the complete available validation suite.



The preferred development loop is:



```text

Implement

&#x20;   ↓

Format

&#x20;   ↓

Static analysis

&#x20;   ↓

Build

&#x20;   ↓

Relevant tests

&#x20;   ↓

Commit

```



For larger milestones:



```text

Clean build

&#x20;   ↓

Unit tests

&#x20;   ↓

Integration tests

&#x20;   ↓

Compatibility tests

&#x20;   ↓

Sanitizers where available

&#x20;   ↓

Commit milestone

```



A failing test must not simply be deleted or weakened to make the build green.



If a test appears incorrect, investigate the expected behavior and document the reason for modifying it.



\---



\# 17. Code Quality and Cleanup Passes



The agent must periodically stop feature development and perform cleanup passes.



Recommended cadence:



\* after approximately 3–5 meaningful feature commits, or

\* after a major subsystem milestone.



A cleanup pass should:



1\. Build the project.

2\. Run tests.

3\. Inspect recently modified code.

4\. Remove dead code.

5\. remove unnecessary duplication.

6\. improve naming.

7\. reduce accidental complexity.

8\. improve interface boundaries.

9\. consolidate repeated version checks.

10\. check ownership and lifetime handling.

11\. run formatting and static analysis.

12\. run tests again.



Cleanup passes must not become endless architecture rewrites.



The rule is:



> Improve the architecture enough to support the next stage of development, but do not rebuild the entire project because a class name annoys you.



Each cleanup pass should normally end with its own commit.



Example:



```text

refactor(core): consolidate backend capability resolution

```



\---



\# 18. Platform-Specific Isolation



Platform-specific code must live in clearly separated modules.



Examples:



```text

platform/

&#x20;   android/

&#x20;   linux/

```



Platform-independent code must not include Android headers unless absolutely necessary.



Similarly, Android API level handling must be isolated.



Preferred:



```text

SharedMemory

&#x20;   ├── AndroidSharedMemoryApi26

&#x20;   ├── AndroidSharedMemoryFallback

&#x20;   └── LinuxSharedMemory

```



Avoid:



```cpp

if (sdk >= 26) {

&#x20;   ...

}

```



spread throughout the project.



A platform-specific decision should ideally happen once during initialization.



The resulting implementation should satisfy a stable interface.



\---



\# 19. Error Handling



OpenGL error behavior is part of compatibility.



The implementation must correctly handle relevant OpenGL errors such as:



\* `GL\_INVALID\_ENUM`

\* `GL\_INVALID\_VALUE`

\* `GL\_INVALID\_OPERATION`

\* `GL\_OUT\_OF\_MEMORY`

\* other applicable errors



Do not allow backend-native errors to directly define frontend OpenGL behavior.



Backend errors should be translated into OpenGL-visible semantics where appropriate.



Validation logic should be testable independently from rendering.



\---



\# 20. Logging and Debugging



Implement structured logging suitable for debugging translation problems.



Useful categories may include:



```text

CORE

STATE

RESOURCE

SHADER

BACKEND

GLES

VULKAN

PLATFORM

EMULATION

```



Debug logging must be configurable.



The release configuration should not spam logs.



The implementation should support diagnosing:



\* selected backend.

\* backend version.

\* detected extensions.

\* selected feature implementations.

\* activated fallbacks.

\* shader translation failures.

\* backend errors.



\---



\# 21. Repository Structure



A recommended structure:



```text

/

├── CMakeLists.txt

├── README.md

├── docs/

│   ├── architecture.md

│   ├── compatibility.md

│   └── feature-matrix.md

│

├── include/

│   └── glcompat/

│

├── src/

│   ├── frontend/

│   ├── core/

│   ├── state/

│   ├── objects/

│   ├── shader/

│   ├── emulation/

│   ├── backend/

│   │   ├── common/

│   │   ├── gles/

│   │   ├── vulkan/

│   │   └── mock/

│   │

│   └── platform/

│       ├── android/

│       └── linux/

│

├── tests/

│   ├── unit/

│   ├── integration/

│   ├── compatibility/

│   └── backend/

│

└── third\_party/

```



The exact layout may evolve, but subsystem boundaries must remain clear.



\---



\# 22. Documentation



Maintain documentation during development.



At minimum:



\## Architecture Documentation



Describe:



\* frontend/backend separation.

\* capability system.

\* resource lifetime model.

\* shader pipeline.

\* platform abstraction.



\## Feature Matrix



Track:



\* OpenGL feature.

\* support status.

\* native implementation.

\* emulation.

\* unsupported limitations.

\* backend requirements.

\* tests.



\## Backend Documentation



Document requirements for implementing a new backend.



A developer should be able to implement a Vulkan backend without reverse-engineering the GLES backend.



\---



\# 23. Development Priorities



Do not attempt the entire OpenGL 4.6 API in arbitrary order.



Prioritize foundational systems first.



Recommended order:



\## Phase 1 — Foundation



\* repository setup.

\* CMake.

\* Git configuration.

\* formatting.

\* static analysis.

\* test framework.

\* backend abstraction.

\* mock backend.

\* capability system.

\* headless Linux environment.



\## Phase 2 — Core OpenGL Objects



\* buffers.

\* textures.

\* shader objects.

\* programs.

\* vertex arrays.

\* framebuffers.

\* renderbuffers.



\## Phase 3 — Core Rendering



\* vertex submission.

\* shader execution.

\* textures.

\* framebuffer rendering.

\* depth.

\* stencil.

\* blending.

\* viewport and scissor.



\## Phase 4 — Shader Translation



\* desktop GLSL ingestion.

\* GLSL ES output.

\* shader validation.

\* transformation pipeline.

\* feature detection.

\* backend adaptation.



\## Phase 5 — Modern OpenGL Features



Gradually implement:



\* UBOs.

\* SSBOs.

\* instancing.

\* transform feedback.

\* image load/store.

\* compute shaders.

\* indirect drawing.

\* synchronization.

\* program pipelines.

\* DSA.



\## Phase 6 — Compatibility and Emulation



Implement missing functionality using:



\* state translation.

\* shader transformation.

\* resource emulation.

\* CPU fallback where reasonable.



\## Phase 7 — Extensions



Evaluate widely used extensions and prioritize based on practical compatibility value.



\## Phase 8 — Backend Expansion



Prepare for and optionally begin:



\* Vulkan backend.

\* improved mock backend.

\* additional test backends.



\---



\# 24. One-Week Autonomous Development Strategy



The agent should not attempt to finish OpenGL 4.6 in one week and then hallucinate success.



The one-week objective is to establish a robust architecture and implement a meaningful, tested compatibility foundation.



Recommended schedule:



\## Day 1



\* inspect repository.

\* configure local Git identity.

\* establish build system.

\* establish test framework.

\* create headless Linux test environment.

\* create initial architecture documentation.

\* create backend interfaces.

\* create mock backend.

\* commit each coherent step.



\## Day 2



\* implement capability system.

\* implement platform abstraction.

\* implement Android API version abstraction strategy.

\* establish SDK 21 fallback patterns.

\* add capability and fallback tests.

\* cleanup pass.

\* commit.



\## Day 3



\* implement core object model.

\* buffers.

\* textures.

\* object lifetime.

\* backend resource abstraction.

\* tests.

\* commit.



\## Day 4



\* implement initial GLES backend.

\* headless backend validation.

\* basic rendering path.

\* framebuffer support.

\* state tracking.

\* tests.

\* commit.



\## Day 5



\* integrate shader manipulation libraries.

\* implement initial shader pipeline.

\* support basic desktop GLSL to backend translation.

\* add shader tests.

\* cleanup pass.

\* commit.



\## Day 6



\* implement selected modern OpenGL functionality.

\* prioritize features required by common applications.

\* begin emulation infrastructure.

\* evaluate useful extensions.

\* document feature matrix.

\* tests.

\* commit.



\## Day 7



\* full cleanup pass.

\* full test run.

\* sanitizer runs where available.

\* architecture review.

\* fix discovered regressions.

\* update documentation.

\* create a final milestone commit.



\---



\# 25. Agent Behavioral Rules



The agent must:



\* inspect existing code before modifying it.

\* avoid unnecessary rewrites.

\* prefer incremental changes.

\* build and test frequently.

\* commit frequently.

\* preserve working states.

\* investigate failures rather than guessing.

\* isolate platform-specific behavior.

\* isolate backend-specific behavior.

\* avoid duplicated version checks.

\* prefer abstractions that represent actual requirements.

\* document important design decisions.

\* keep compatibility limitations explicit.



The agent must not:



\* fake OpenGL feature support.

\* remove tests merely because they fail.

\* silently ignore unsupported API calls.

\* scatter Android SDK checks across unrelated code.

\* hardwire the frontend to GLES.

\* perform massive uncommitted rewrites.

\* replace working systems without justification.

\* claim completion without tests.

\* reduce the advertised compatibility target merely to simplify implementation.



\---



\# 26. Definition of Progress



Progress is not measured by lines of code or the number of OpenGL functions declared.



Progress is measured by:



```text

Implemented

\+

Tested

\+

Documented

\+

Integrated

\+

Committed

```



A feature is only considered implemented when:



1\. The frontend exposes the intended behavior.

2\. The backend path works or a documented emulation exists.

3\. Capability handling is correct.

4\. relevant fallback paths are tested.

5\. automated tests pass.

6\. the implementation is committed.



\---



\# 27. Definition of Done for the Initial Autonomous Run



At the end of the initial development period, the project should ideally have:



\* a clean C++ architecture.

\* a backend-independent frontend.

\* a functional GLES backend foundation.

\* a mock or test backend.

\* headless Linux testing.

\* Android SDK 21 compatibility strategy.

\* centralized platform capability handling.

\* centralized graphics capability handling.

\* tested fallback abstractions.

\* a shader translation pipeline foundation.

\* meaningful OpenGL API functionality.

\* a maintained feature matrix.

\* regular meaningful Git history.

\* periodic cleanup commits.

\* documented architecture.

\* no knowingly broken commits at the final milestone.



The agent must provide an honest final status report distinguishing:



```text

Implemented and tested

Implemented but incomplete

Emulated

Planned

Unsupported

Blocked

```



It must not describe the project as “OpenGL 4.6 compatible” unless that claim is supported by the actual implementation and tests.



\---



\# Final Instruction



Treat this project as a long-running systems project, not a one-shot code generation task.



Prioritize correctness, reversibility, testability, and architectural boundaries.



Make small changes.



Test them.



Commit them.



Periodically clean up the mess you created.



Then continue making the next mess.



\# 28. Persistent Development Journal and Progress Tracking



The agent must maintain a persistent, version-controlled development journal throughout the entire project.



Create:



```text

docs/agent-progress.md

```



This file is part of the project and must be committed to Git alongside the implementation.



The purpose of this file is to ensure that development can be resumed, audited, understood, or continued by another agent or human without relying on conversation history.



\## Required Contents



The file should maintain the following sections:



\### Current Status



A concise description of the current state of the project.



Example:



```text

Current milestone:

Core GLES rendering foundation



Overall status:

Early implementation



Last updated:

2026-08-26



Known major blockers:

\- Shader translation for geometry shaders is not implemented.

\- Context sharing semantics still need investigation.

```



\### Completed



Record meaningful completed work.



For each item, include:



\* what was implemented.

\* relevant subsystem.

\* associated tests.

\* commit hash where useful.



Example:



```text

\- \[x] Backend capability abstraction

&#x20; - Added centralized feature detection.

&#x20; - Added unit tests for capability resolution.

&#x20; - Commit: abc1234

```



Do not list trivial implementation details such as every renamed variable.



\### In Progress



Record work currently being actively developed.



Each item should explain:



\* what is being worked on.

\* what has already been completed.

\* what remains.

\* any known issues.



Example:



```text

\- \[ ] GLES framebuffer implementation

&#x20; - Basic framebuffer creation works.

&#x20; - Attachment validation is implemented.

&#x20; - Multisample handling remains.

&#x20; - Current concern: GLES framebuffer completeness rules differ from desktop GL.

```



\### TODO



Maintain a prioritized TODO list.



Use priorities such as:



```text

P0 — Blocking / critical

P1 — Important

P2 — Normal

P3 — Nice to have

```



Example:



```text

\- \[ ] P0: Implement framebuffer completeness translation

\- \[ ] P1: Add shader translation tests for sampler arrays

\- \[ ] P1: Implement Android API 21 synchronization fallback

\- \[ ] P2: Add extension capability reporting

\- \[ ] P3: Improve debug logging

```



TODO items should be removed or marked complete when finished.



Do not allow the TODO list to become a graveyard of obsolete ideas.



\### Known Issues



Record known bugs, limitations, questionable behavior, and technical debt.



Each issue should include enough information for another developer to reproduce or investigate it.



Example:



```text

\- GLES backend occasionally produces incorrect depth results when depth

&#x20; and stencil attachments are emulated separately.

&#x20; Reproduction test: tests/integration/depth\_stencil.cpp

```



\### Architecture Decisions



Record significant architectural decisions made during development.



For example:



```text

Decision:

Platform-specific API handling is centralized behind platform interfaces.



Reason:

Avoid SDK-version checks being scattered throughout the renderer.



Consequence:

Android API-level differences should generally only exist inside

platform implementations.

```



Do not document every trivial decision.



Focus on decisions that another developer might otherwise question.



\### Compatibility Progress



Summarize the current OpenGL compatibility status.



This may reference the detailed feature matrix but should provide a quick overview.



Example:



```text

OpenGL 4.6:

&#x20; Core API: partial

&#x20; Compatibility profile: minimal

&#x20; Shader stages: partial

&#x20; DSA: partial

&#x20; Compute: supported through GLES 3.1 where available

&#x20; Tessellation: unsupported/emulated where possible

```



The detailed information belongs in:



```text

docs/feature-matrix.md

```



\### Recent Work



Maintain a short chronological record of recent development.



Example:



```text

2026-08-26

\- Added backend abstraction.

\- Added mock backend.

\- Added capability system.

\- Added initial headless Linux test harness.



2026-08-27

\- Implemented buffer object abstraction.

\- Added GLES buffer implementation.

\- Added buffer lifetime tests.

```



Keep this concise.



This is a development journal, not a diary.



\### Next Steps



At the end of every meaningful work session, update this section with the next logical tasks.



It should answer:



> "If I stopped working right now and another agent took over, what should they do next?"



Example:



```text

1\. Finish GLES framebuffer attachments.

2\. Add framebuffer completeness tests.

3\. Test the implementation using the headless Linux backend.

4\. Commit the completed framebuffer milestone.

5\. Begin shader compiler integration.

```



\---



\# Journal Maintenance Rules



The agent must update `docs/agent-progress.md`:



\* after completing a meaningful milestone.

\* after discovering an important bug.

\* after making a significant architectural decision.

\* after changing priorities.

\* before ending a development session.

\* before handing the project to another agent.

\* during cleanup passes when the architecture or implementation status changes.



The journal must be committed regularly.



A journal update should normally accompany the implementation commit it describes.



For example:



```text

feat(buffer): implement backend buffer abstraction

```



should include the corresponding progress update when appropriate.



\---



\# Resume Protocol



At the beginning of every autonomous development session, the agent must inspect:



```text

docs/agent-progress.md

docs/feature-matrix.md

git log

git status

```



before deciding what to work on.



The agent should then:



1\. Determine the last known state.

2\. Check whether the working tree contains unfinished work.

3\. Inspect recent commits.

4\. Run relevant tests if necessary.

5\. Continue from the documented next steps.



Do not blindly start implementing a new feature without first understanding the current project state.



\---



\# Honest Progress Reporting



The journal must describe reality rather than intentions.



Do not write:



```text

\[x] OpenGL 4.6 support

```



merely because the corresponding interfaces exist.



Instead distinguish between:



```text

Implemented

Implemented and tested

Partially implemented

Emulated

Backend-dependent

Known broken

Not implemented

```



A large number of declared API functions with no meaningful implementation does not constitute progress.



Likewise, generated boilerplate does not count as a completed feature.



\---



\# TODO Discipline



The agent should periodically review the TODO list.



During cleanup passes:



\* remove obsolete TODOs.

\* merge duplicates.

\* reprioritize tasks.

\* mark completed work.

\* convert vague TODOs into actionable tasks.

\* record blockers.

\* avoid accumulating speculative tasks that are unlikely to matter.



Bad:



```text

TODO: fix graphics stuff

```



Good:



```text

P1: Implement GLES fallback for immutable texture storage.

Affected subsystem: backend/gles/texture

Required by: glTexStorage\* compatibility tests

```



\---



\# Git and Journal Consistency



The journal must never claim that work exists when the corresponding implementation has been lost or reverted.



If a commit is reverted:



1\. Update the journal.

2\. Mark the affected functionality appropriately.

3\. Record the reason if significant.

4\. Update TODOs if the work needs to be attempted again.



Git history and the development journal should together provide a reliable record of the project's evolution.



The goal is that a developer can inspect:



```text

docs/agent-progress.md

&#x20;       +

docs/feature-matrix.md

&#x20;       +

git log

&#x20;       +

tests/

```



and understand what the project currently does, what it does not do, why major architectural decisions were made, and what should happen next.



