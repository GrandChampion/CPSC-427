#pragma once

#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include <gl3w.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define NOMINMAX

#include <GLFW/glfw3.h>

#include <glm/vec2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

using namespace glm;

#include "tiny_ecs.hpp"

#include "../ext/project_path.hpp"

inline std::string data_path() { return std::string(PROJECT_SOURCE_DIR) + "data"; };

inline std::string shader_path(const std::string &name) {
    return std::string(PROJECT_SOURCE_DIR) + "/shaders/" + name;
};

inline std::string textures_path(const std::string &name) { return data_path() + "/textures/" + std::string(name); };

inline std::string audio_path(const std::string &name) { return data_path() + "/audio/" + std::string(name); };

inline std::string mesh_path(const std::string &name) { return data_path() + "/meshes/" + std::string(name); };

const int window_width_px = 1200;
const int window_height_px = 800;
const float scene_width_px = 6000;
const float scene_height_px = 6000;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct Transform {
    mat3 mat = {{1.f, 0.f, 0.f},
                {0.f, 1.f, 0.f},
                {0.f, 0.f, 1.f}};

    void scale(vec2 scale);

    void rotate(float radians);

    void translate(vec2 offset);
};

bool gl_has_errors();
