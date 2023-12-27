#include "physics_system.hpp"
#include <cmath>
#include "world_system.hpp"

using namespace glm;

const float G = 10000.f;

PhysicsSystem physics_system;

vec2 get_gravity_effect(Motion &motion, Entity &entity) {
    vec2 total_gravity(0.0f, 0.0f);
    if (registry.ignore_physics.has(entity))
        return total_gravity;

    auto &motion_container = registry.motions;
    for (uint j = 0; j < motion_container.size(); j++) {
        Entity &other_entity = motion_container.entities[j];
        Motion &other_motion = motion_container.components[j];
        if (other_entity == entity || registry.ignore_physics.has(other_entity))
            continue;

        vec2 dp = other_motion.position - motion.position;
        float dist_squared = dot(dp, dp);
        float force_magnitude = (G * other_motion.mass) / dist_squared;
        vec2 force_direction = normalize(dp);
        vec2 gravitational_force = force_magnitude * force_direction;
        total_gravity += gravitational_force;
    }

    return total_gravity;
}

bool collides(Entity missileEn, const Motion &motion_missile, const Motion &motion_other) {
    Mesh *missileMesh = registry.meshPtrs.get(missileEn);

    for (const ColoredVertex &vertex: missileMesh->vertices) {
        vec2 vertexPosition = vec2(vertex.position.x, vertex.position.y);
        vec2 dp = motion_missile.position + vertexPosition - motion_other.position;
        double distSq = dot(dp, dp);
        double radSq = pow(motion_other.radius, 2);
        if (distSq < radSq)
            return true;
    }
    return false;
}

void PhysicsSystem::step(float elapsed_ms) {
    Phase &p = registry.phases.components[0];
    if (!p.simulation)
        return;

    auto &motion_container = registry.motions;
    float step_seconds = elapsed_ms / 1000.f;
    for (uint i = 0; i < motion_container.size(); i++) {
        Motion &motion = motion_container.components[i];
        Entity &entity = motion_container.entities[i];
        float speed_boost = 1.0f;
        if (registry.speed_up.has(entity)) {
            speed_boost = registry.speed_up.get(entity).boost;
        }

        if (registry.angular_motions.has(entity)) {
            Transform transform;
            transform.rotate(step_seconds * motion.angle);
            motion.position = mat2(transform.mat) * (motion.position - motion.velocity) + motion.velocity * speed_boost;
        } else {
            vec2 total_gravity = get_gravity_effect(motion, entity);
            motion.velocity += total_gravity * step_seconds;
            motion.position += (motion.velocity) * step_seconds * speed_boost;
            if (registry.asteroids.has(entity))
                motion.angle += step_seconds * M_PI / 2.f;
            else if (dot(motion.velocity, motion.velocity) > 0)
                motion.angle = atan2(motion.velocity.y, motion.velocity.x);
        }
    }

    auto &missile_container = registry.missiles;
    for (uint i = 0; i < missile_container.components.size(); i++) {
        Motion &motion_missile = registry.motions.get(missile_container.entities[i]);
        Entity entity_missile = missile_container.entities[i];
        for (uint j = 0; j < motion_container.components.size(); j++) {
            Motion &motion_other = motion_container.components[j];
            Entity entity_other = motion_container.entities[j];
            if (registry.missiles.has(entity_other) || registry.ignore_physics.has(entity_other))
                continue;
            if (collides(entity_missile, motion_missile, motion_other)) {
                Mix_PlayChannel(-1, world_system.missile_destroyed_sound, 0);
                registry.collisions.emplace_with_duplicates(entity_missile, entity_other);
                registry.collisions.emplace_with_duplicates(entity_other, entity_missile);
            }
        }
    }
}