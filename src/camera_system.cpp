#include "camera_system.hpp"

#include "callback_system.hpp"
#include "tiny_ecs_registry.hpp"


CameraSystem camera_system;

void CameraSystem::init(GLFWwindow *window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    float preferred_ratio = window_width_px / window_height_px;
    float screen_ratio = ((float) width) / ((float) height);
    if (screen_ratio > preferred_ratio) {
        camera_size.x = window_height_px * screen_ratio;
        camera_size.y = window_height_px;
    } else {
        camera_size.y = window_width_px / screen_ratio;
        camera_size.x = window_width_px;
    }
    enforce_limits();
    callback_system.add_on_mouse_move_callback([](GLFWwindow *w, double, double) { camera_system.on_mouse_move(w); });
    callback_system.add_on_scroll_callback(
            [](GLFWwindow *w, double, double offset) { camera_system.on_scroll(w, (float) offset); });
}


const float move_timer_duration = 800.f;
float move_timer = 0.f;

void CameraSystem::step(float elapsed_ms) {
    float camera_speed = camera_size.y + camera.z;
    vec3 change = camera_speed * (elapsed_ms / 1000.f) * vec3(camera_velocity, 0.f);
    if (locked_on && registry.motions.has(locked_on_entity)) {
        change = vec3(registry.motions.get(locked_on_entity).position, 10.f) - camera;
        if (move_timer > 0.f) {
            if (elapsed_ms < move_timer)
                change *= elapsed_ms / move_timer;
            move_timer -= elapsed_ms;
        }
    }
    camera += change;
    enforce_limits();
}

void CameraSystem::enforce_limits() {
    vec2 limits = {
            (scene_width_px - camera_size.x) / 2.f - camera.z,
            (scene_height_px - camera_size.y) / 2.f - camera.z * camera_size.y / camera_size.x
    };
    camera.x = max(-limits.x, min(camera.x, limits.x));
    camera.y = max(-limits.y, min(camera.y, limits.y));
}

void CameraSystem::on_scroll(GLFWwindow *w, float offset) {
    if (move_timer > 0.f) return;
    vec2 mouse_pos = camera_system.get_mouse_on_camera(w);
    const float zoom_speed = 50.f;

    float max_zoom = min(scene_width_px - camera_size.x, scene_height_px - camera_size.y) / 2.f;
    float z_change = max(-camera.z, min(-zoom_speed * offset, max_zoom - camera.z));

    const vec2 mouse_pos_on_camera = vec2(2.f * mouse_pos.x - 1.f, 1.f - 2.f * mouse_pos.y);
    camera += z_change * vec3(-(camera_size / camera_size.x) * mouse_pos_on_camera, 1.f);
    locked_on = false;
    enforce_limits();
}

void CameraSystem::on_mouse_move(GLFWwindow *w) {
    if (move_timer > 0.f) return;
    vec2 mouse_position = camera_system.get_mouse_on_camera(w);
    float camera_threshold = 0.05f;

    float left = mouse_position.x < camera_threshold;
    float right = mouse_position.x > 1 - camera_threshold;
    float up = mouse_position.y < camera_threshold;
    float down = mouse_position.y > 1 - camera_threshold;

    camera_velocity = vec2(right - left, up - down);
    if (dot(camera_velocity, camera_velocity) > 0) locked_on = false;
}

mat3 CameraSystem::get_projection_matrix(bool hud) {
    vec2 scene_size = (1.f + 2.f * (hud ? 0 : camera.z) / camera_size.x) * camera_size;
    Transform t;
    t.scale(vec2(2.f, 2.f) / scene_size);
    if (!hud) t.translate(-vec2(camera));
    return t.mat;
}

vec2 CameraSystem::get_mouse_on_camera(GLFWwindow *window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    xpos /= width;
    ypos /= height;
    return vec2(xpos, ypos);
}

void CameraSystem::lock_on(Entity e) {
    move_timer = move_timer_duration;
    locked_on = true;
    locked_on_entity = e;
}