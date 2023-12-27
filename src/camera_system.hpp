#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"

class CameraSystem {
public:
    void init(GLFWwindow *window);

    void step(float elapsed_ms);

    mat3 get_projection_matrix(bool hud);

    vec2 get_mouse_on_camera(GLFWwindow *window);

    void on_scroll(GLFWwindow *w, float offset);

    void on_mouse_move(GLFWwindow *w);

    vec2 camera_size = {1200.f, 800.f};

    void lock_on(Entity e);

private:
    void enforce_limits();

    bool locked_on;
    Entity locked_on_entity;
    vec3 camera = {0.f, 0.f, 1200.f};
    vec2 camera_velocity = {0.f, 0.f};
};

extern CameraSystem camera_system;