#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

const glm::mat4& camera_view_inverse();
const glm::mat4& camera_projection_view();

[[nodiscard]] bool camera_inside_frustrum_transform(
    const glm::vec3& minimum,
    const glm::vec3& maximum,
    const glm::mat4& transform
);

[[nodiscard]] glm::vec3 camera_eye_position();
[[nodiscard]] glm::vec3 camera_position();

const glm::mat4& camera_view_matrix();
const glm::mat4& camera_projection_matrix();

[[nodiscard]] glm::vec2 input_mouse_position();
[[nodiscard]] bool input_mouse_moved();

void camera_set_position(
    const glm::vec3& position
);

void camera_update(
    double delta
);

void input_set_mouse_world(
    const glm::vec3& position
);

void input_update_mouse_world(
    const glm::vec3& position
);
