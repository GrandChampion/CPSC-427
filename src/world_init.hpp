#pragma once

#include <random>
#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"

const float FISH_BB_WIDTH = 0.4f * 296.f;
const float FISH_BB_HEIGHT = 0.4f * 165.f;
const float TURTLE_BB_WIDTH = 0.4f * 300.f;
const float TURTLE_BB_HEIGHT = 0.4f * 202.f;

Entity createAsteroid(std::uniform_real_distribution<float> &uniform_dist, std::default_random_engine &rng);

Entity createBackground();

Entity createParticle(vec2 inputPosition, float scale);

Entity createAimer(vec2 pos, vec2 direction);

void createMissile(Motion motion, vec2 direction, VARIANT m_type);

Entity createHUDComponent(vec2 pos, vec2 scale, RenderRequest request, int phases = WorldPhase::GAME);

Entity createHUDComponent(vec2 pos, vec2 scale, TEXTURE_ASSET_ID texture, int phases = WorldPhase::GAME);

Entity createSun(vec2 pos, float scale_factor, float mass);

Entity createPlanet(vec2 pos, float scale_factor, float mass, vec3 color, float angular_velocity);

Entity createWormhole(vec2 pos, float scale_factor, float mass);