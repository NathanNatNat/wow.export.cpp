/*!
    wow.export (https://github.com/Kruithne/wow.export)
    Authors: Kruithne <kruithne@gmail.com>
    License: MIT
*/
#include "renderer_utils.h"
#include <array>
#include <cstring>
#include <vector>

namespace renderer_utils {

static constexpr std::array<float, 16> IDENTITY_MAT4 = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

GLuint create_bones_ssbo(size_t bone_count) {
    GLuint ssbo = 0;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    if (bone_count > 0) {
        std::vector<float> initial(bone_count * 16, 0.0f);
        for (size_t i = 0; i < bone_count; i++)
            std::memcpy(&initial[i * 16], IDENTITY_MAT4.data(), 16 * sizeof(float));
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     static_cast<GLsizeiptr>(bone_count * 16 * sizeof(float)),
                     initial.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}

} // namespace renderer_utils
