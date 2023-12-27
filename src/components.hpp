#pragma once

#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

typedef enum {
    STANDARD = 1,
    CLUSTER = 2,
    FAST = 3,
    GRAVITY = 4
} VARIANT;

enum WorldPhase {
    WELCOME = 1,
    TUT1 = 2 * WELCOME,
    TUT2 = 2 * TUT1,
    TUT3 = 2 * TUT2,
    GAME = 2 * TUT3,
    END = 2 * GAME
};


struct Asteroid {
};
struct Player {
};
struct Sun {
};
struct IgnorePhysics {
};
struct SmokeParticle {
};
struct AngularMotion {
};
struct PlanetName {
};
struct Wormhole {
};
struct Hide {
};

struct Missile {
    float damage;
    VARIANT variant;
};

struct Planet {
    float old_life = 100.f;
    float time_since_change = 0.f;
    float life = 100.f;
    vec3 color = {0.f, 0.f, 1.f};
};

struct Phase {
    int player = 0;
    bool simulation = false;
    WorldPhase phase = WorldPhase::WELCOME;
};

struct Timer {
    bool simulation = true;
    float ms = 1000.f;
    bool death = false;
};

struct SpeedUp {
    float boost = 2.3f;
    float ms = 5000.f;
    float decay_factor = 0.05f;
};

struct Motion {
    vec2 position = {0.f, 0.f};
    vec2 velocity = {0.f, 0.f};
    vec2 scale = {10.f, 10.f};
    float angle = 0.f;
    float mass = 1.f;
    float radius = 0;
};

struct Collision {
    Entity other_entity;

    Collision(Entity &other_entity) { this->other_entity = other_entity; };
};

struct Debug {
    bool in_debug_mode = 0;
    bool in_freeze_mode = 0;
};
extern Debug debugging;

struct ScreenState {
    float screen_darken_factor = -1;
};

struct DebugComponent {
};

struct ColoredVertex {
    vec3 position;
    vec3 color;

    ColoredVertex(vec3 p, vec3 c) {
        position = p;
        color = c;
    }
};

struct TexturedVertex {
    vec3 position;
    vec2 texcoord;
};

struct Animation {
    float total_elapsed;
    float frame_duration;
    float nx_frame;
    float ny_frame;

    bool does_loop;
    bool is_active;
    bool is_flipped;
};

struct HUDComponent {
    int phases = 1;
};

struct Mesh {
    static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex> &out_vertices,
                                std::vector<uint16_t> &out_vertex_indices, vec2 &out_size);

    vec2 original_size = {1, 1};
    std::vector<ColoredVertex> vertices;
    std::vector<uint16_t> vertex_indices;
};

enum class TEXTURE_ASSET_ID {
    ICE = 0,
    SUN = ICE + 1,
    KEY1 = SUN + 1,
    KEY2 = KEY1 + 1,
    KEY3 = KEY2 + 1,
    KEY4 = KEY3 + 1,
    ASTEROID = KEY4 + 1,
    ASTEROID2 = ASTEROID + 1,
    ASTEROID3 = ASTEROID2 + 1,
    BACKGROUND = ASTEROID3 + 1,
    WORMHOLE = BACKGROUND + 1,
    HIGHLIGHT = WORMHOLE + 1,
    TEXTURE_COUNT = HIGHLIGHT + 1,
};
const int texture_count = (int) TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID {
    COLOURED = 0,
    PEBBLE = COLOURED + 1,
    MISSILE = PEBBLE + 1,
    SMOKE = MISSILE + 1,
    TEXTURED = SMOKE + 1,
    ANIMATED = TEXTURED + 1,
    WATER = ANIMATED + 1,
    EFFECT_COUNT = WATER + 1
};
const int effect_count = (int) EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID {
    MISSILE = 0,
    FAST_MISSILE = MISSILE + 1,
    CLUSTER_MISSILE = FAST_MISSILE + 1,
    GRAVITY_MISSILE = CLUSTER_MISSILE + 1,
    SMOKE = GRAVITY_MISSILE + 1,
    SPRITE = SMOKE + 1,
    PEBBLE = SPRITE + 1,
    DEBUG_LINE = PEBBLE + 1,
    SCREEN_TRIANGLE = DEBUG_LINE + 1,
    GEOMETRY_COUNT = SCREEN_TRIANGLE + 1
};
const int geometry_count = (int) GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest {
    TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
    EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
    GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};