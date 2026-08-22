#include "camera_view.h"

#include "camera.h"

const glm::mat4& camera_view_inverse() {
    return camera.view_inverse;
}

const glm::mat4& camera_projection_view() {
    return camera.projection_view;
}

bool camera_inside_frustrum_transform(
    const glm::vec3& minimum,
    const glm::vec3& maximum,
    const glm::mat4& transform
) {
    return camera.inside_frustrum_transform(
        minimum,
        maximum,
        transform
    );
}

glm::vec3 camera_eye_position() {
    return camera.position
        - camera.direction * camera.distance;
}

glm::vec3 camera_position() {
    return camera.position;
}

const glm::mat4& camera_view_matrix() {
    return camera.view;
}

const glm::mat4& camera_projection_matrix() {
    return camera.projection;
}

glm::vec2 input_mouse_position() {
    return input_handler.mouse;
}

bool input_mouse_moved() {
    return input_handler.mouse
        != input_handler.previous_mouse;
}

void camera_set_position(
    const glm::vec3& position
) {
    camera.position = position;
}

void camera_update(
    const double delta
) {
    camera.update(delta);
}

void input_set_mouse_world(
    const glm::vec3& position
) {
    input_handler.mouse_world = position;
}

void input_update_mouse_world(
    const glm::vec3& position
) {
    input_handler.previous_mouse_world =
        input_handler.mouse_world;
    input_handler.mouse_world = position;
}
