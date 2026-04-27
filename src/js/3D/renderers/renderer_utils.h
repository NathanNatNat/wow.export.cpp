/*!
    wow.export (https://github.com/Kruithne/wow.export)
    Authors: Kruithne <kruithne@gmail.com>
    License: MIT
*/
#pragma once

#include <glad/gl.h>
#include <cstddef>

namespace renderer_utils {
    // C++ equivalent of JS create_bones_ubo.
    // JS uses WebGL2 UBOs; desktop OpenGL 4.3 uses SSBO for bone matrices.
    // Creates an SSBO of bone_count * 64 bytes (16 floats per 4x4 matrix),
    // initialised with identity matrices. Returns the GL buffer handle.
    GLuint create_bones_ssbo(size_t bone_count);
} // namespace renderer_utils
