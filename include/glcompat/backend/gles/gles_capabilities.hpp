#pragma once

#include "glcompat/core/capabilities_table.hpp"

namespace glcompat {

struct GLESLib;

// Populate a CapabilityTable from a detected GLES version/extension set.
// Honest: only marks features Native/Emulated when this GLES version actually
// provides them; geometry/tessellation have no GLES equivalent (Unsupported),
// DSA is core only in desktop GL (Unsupported here unless an ext is present).
void populateGLESCapabilities(CapabilityTable& table, const GLESLib& lib);

} // namespace glcompat
