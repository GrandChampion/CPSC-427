#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"
#include "world_system.hpp"

Entity createMotionEntity(
        RenderRequest request,
        vec2 position, vec2 velocity, vec2 scale,
        float angle = 0.f, float mass = 1.f, float radius = 0) {
    Entity entity;
    Motion &motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.velocity = velocity;
    motion.scale = scale;
    motion.angle = angle;
    motion.mass = mass;
    motion.radius = radius;

    registry.renderRequests.insert(entity, request);
    return entity;
}

Entity createMotionEntity(
        RenderRequest request,
        vec2 position, vec2 velocity, float scale,
        float angle = 0.f, float mass = 1.f, float radius = 0) {

    Mesh &mesh = render_system.getMesh(request.used_geometry);
    return createMotionEntity(request, position, velocity, scale * mesh.original_size, angle, mass, radius);
}

Entity createAngularMotionEntity(
        RenderRequest request,
        vec2 position, vec2 centre, float angular_velocity, float scale,
        float mass = 1.f, float radius = 0) {
    auto e = createMotionEntity(request, position, centre, scale, angular_velocity, mass, radius);
    registry.angular_motions.emplace(e);
    return e;
}

Entity createAsteroid(std::uniform_real_distribution<float> &uniform_dist, std::default_random_engine &rng) {
    float x = -5000.f + 10000.f * uniform_dist(rng);
    float y = -5000.f + 10000.f * uniform_dist(rng);

    int x_sign = x > 0 ? 1 : -1;
    int y_sign = y > 0 ? 1 : -1;

    float velocity = 100.f + 200.f * uniform_dist(rng);

    float input_scale = 100.f + 200.f * uniform_dist(rng);

    float ra = floorf(uniform_dist(rng) * 10.f);

    Entity entity;
    if (ra < 3.f) {
        entity = createMotionEntity(
                {TEXTURE_ASSET_ID::ASTEROID, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE},
                {x, y}, {x_sign * -1.f * velocity, y_sign * -1.f * velocity}, input_scale);
    } else if (ra < 6.f) {
        entity = createMotionEntity(
                {TEXTURE_ASSET_ID::ASTEROID2, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE},
                {x, y}, {x_sign * -1.f * velocity, y_sign * -1.f * velocity}, input_scale);
    } else {
        entity = createMotionEntity(
                {TEXTURE_ASSET_ID::ASTEROID3, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE},
                {x, y}, {x_sign * -1.f * velocity, y_sign * -1.f * velocity}, input_scale);
    }

    registry.asteroids.emplace(entity);
    registry.ignore_physics.emplace(entity);
    Timer t = registry.timers.emplace(entity);
    t.ms = 5000.f;

    return entity;
}

Entity createParticle(vec2 position, float scale) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-10.f, 10.f);
    float randomX = dis(gen);
    float randomY = dis(gen);

    std::uniform_real_distribution<> dis2(0.1f, 1.f);
    float randomScale = dis2(gen);

    auto entity = createMotionEntity(
            {TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::SMOKE, GEOMETRY_BUFFER_ID::SMOKE},
            position, {randomX, randomY}, scale * randomScale);

    registry.smoke_trail.emplace(entity);
    registry.ignore_physics.emplace(entity);
    Timer &t = registry.timers.emplace(entity);
    t.death = true;
    return entity;
}

Entity createHUDComponent(vec2 pos, vec2 scale, RenderRequest request, int phases) {
    auto entity = createMotionEntity(request, pos, {0.f, 0.f}, scale);
    HUDComponent &hud = registry.huds.emplace(entity);
    hud.phases = phases;
    registry.ignore_physics.emplace(entity);
    return entity;
}

Entity createHUDComponent(vec2 pos, vec2 scale, TEXTURE_ASSET_ID texture, int phases) {
    return createHUDComponent(pos, scale, {texture, EFFECT_ASSET_ID::TEXTURED, GEOMETRY_BUFFER_ID::SPRITE}, phases);
}

Entity createAimer(vec2 pos, vec2 direction) {
    auto entity = createMotionEntity(
            {TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::MISSILE, GEOMETRY_BUFFER_ID::MISSILE},
            pos, {0.f, 0.f}, 10.f, atan2(direction.y, direction.x) + M_PI);

    registry.ignore_physics.emplace(entity);
    registry.colors.insert(entity, {0.2f, 0.2f, 0.2f});

    return entity;
}

Entity createBackground() {
    auto entity = Entity();

    Mesh &mesh = render_system.getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    Motion &motion = registry.motions.emplace(entity);
    motion.position = {0.f, 0.f};

    float background_width = 25056.f;
    float background_height = 15900.f;

    float preferred_ratio = scene_width_px / scene_height_px;
    float bg_ratio = background_width / background_height;
    if (bg_ratio > preferred_ratio) {
        background_width = scene_height_px * bg_ratio;
        background_height = scene_height_px;
    } else {
        background_height = scene_width_px / bg_ratio;
        background_width = scene_width_px;
    }

    motion.scale = {background_width, background_height};

    registry.ignore_physics.emplace(entity);
    registry.renderRequests.insert(
            entity,
            {TEXTURE_ASSET_ID::BACKGROUND,
             EFFECT_ASSET_ID::TEXTURED,
             GEOMETRY_BUFFER_ID::SPRITE});

    return entity;
}

Entity createMissile(vec2 pos, vec2 velocity, float scale, float damage, GEOMETRY_BUFFER_ID geometry) {
    auto entity = createMotionEntity(
            {TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::MISSILE, geometry},
            pos, velocity, scale, atan2(velocity.y, velocity.x) + M_PI, 0.000000000000000000000001f);

    Mesh &mesh = render_system.getMesh(GEOMETRY_BUFFER_ID::MISSILE);
    registry.meshPtrs.emplace(entity, &mesh);

    Missile &m = registry.missiles.emplace(entity);
    m.damage = damage;

    SpeedUp &s = registry.speed_up.emplace(entity);

    Mix_PlayChannel(-1, world_system.missile_fire_sound, 0);

    return entity;
}

const float MISSILE_SPEED = 300.f;

void createMissile(Motion planet_motion, vec2 direction, VARIANT m_type) {

    int count = m_type == VARIANT::CLUSTER ? 3 : 1;

    float scale = m_type == VARIANT::CLUSTER ? 10.f : 30.f;
    float radius_minus_missile = 50.f + planet_motion.scale.y - scale;
    for (int i = 0; i < count; i++) {
        Transform t;
        if (count != 1) {
            t.rotate((M_PI / 8.f) * (count - i) / count);
        }
        vec2 missile_dir = vec2(t.mat * vec3(direction, 1.f));
        vec2 pos = planet_motion.position + radius_minus_missile * direction + scale * missile_dir;
        vec2 velocity = MISSILE_SPEED * missile_dir;
        GEOMETRY_BUFFER_ID geometry = GEOMETRY_BUFFER_ID::MISSILE;
        float damage = 50.f;
        if (m_type == VARIANT::FAST) {
            velocity *= 3.f;
            geometry = GEOMETRY_BUFFER_ID::FAST_MISSILE;
            damage = 40.f;
        } else if (m_type == VARIANT::CLUSTER) {
            geometry = GEOMETRY_BUFFER_ID::CLUSTER_MISSILE;
            damage = 25.f;
        } else if (m_type == VARIANT::GRAVITY) {
            geometry = GEOMETRY_BUFFER_ID::GRAVITY_MISSILE;
        }
        auto entity = createMissile(pos, velocity, scale, damage, geometry);
        registry.colors.insert(entity, {0.f, 0.8f, 0.8f});
        if (m_type == VARIANT::GRAVITY) {
            Motion &motion = registry.motions.get(entity);
            motion.mass = 3000.f;
        }
    }
}

Entity createSun(vec2 pos, float scale_factor, float mass) {
    auto entity = createAngularMotionEntity(
            {TEXTURE_ASSET_ID::SUN, EFFECT_ASSET_ID::ANIMATED, GEOMETRY_BUFFER_ID::SPRITE},
            pos, {0.f, 0.f}, 0.f, scale_factor, mass, scale_factor);
    registry.suns.emplace(entity);

    Animation &anime = registry.animations.emplace(entity);
    anime.does_loop = true;
    anime.frame_duration = 100.f;
    anime.nx_frame = 20.f;
    anime.ny_frame = 20.f;

    return entity;
}

Entity createWormhole(vec2 pos, float scale_factor, float mass) {
    auto entity = createAngularMotionEntity(
            {TEXTURE_ASSET_ID::WORMHOLE, EFFECT_ASSET_ID::ANIMATED, GEOMETRY_BUFFER_ID::SPRITE},
            pos, pos, 0.f, scale_factor, mass, scale_factor
    );
    registry.wormholes.emplace(entity);

    Animation &anime = registry.animations.emplace(entity);
    anime.does_loop = true;
    anime.frame_duration = 100.f;
    anime.nx_frame = 20.f;
    anime.ny_frame = 20.f;
    Motion &motion = registry.motions.get(entity);
    motion.radius = 100.f;
    return entity;
}

Entity createPlanet(vec2 pos, float scale_factor, float mass, vec3 color, float angular_velocity) {
    auto entity = createAngularMotionEntity(
            {TEXTURE_ASSET_ID::ICE, EFFECT_ASSET_ID::ANIMATED, GEOMETRY_BUFFER_ID::SPRITE},
            pos, {0.f, 0.f}, angular_velocity, 2 * scale_factor, mass, scale_factor);
    Planet &p = registry.planets.emplace(entity);
    p.color = color;

    Animation &anime = registry.animations.emplace(entity);
    anime.does_loop = true;
    anime.frame_duration = 100.f;
    anime.nx_frame = 20.f;
    anime.ny_frame = 20.f;
    return entity;
}
