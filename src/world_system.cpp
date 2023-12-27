#include "world_system.hpp"
#include "world_init.hpp"

#include <cassert>

#include "camera_system.hpp"
#include "callback_system.hpp"
#include "physics_system.hpp"
#include <cmath>
#include <unordered_set>
#include "components.hpp"
#include <GLFW/glfw3.h>

const size_t MAX_ASTEROIDS = 29;

WorldSystem world_system;

WorldSystem::WorldSystem() {
    rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
    if (background_music != nullptr)
        Mix_FreeMusic(background_music);
    if (missile_fire_sound != nullptr)
        Mix_FreeChunk(missile_fire_sound);
    if (missile_destroyed_sound != nullptr)
        Mix_FreeChunk(missile_destroyed_sound);
    if (game_over_sound != nullptr)
        Mix_FreeChunk(game_over_sound);
    Mix_CloseAudio();

    registry.clear_all_components();
}

namespace {
    void glfw_err_cb(int error, const char *desc) {
        fprintf(stderr, "%d: %s", error, desc);
    }
}

GLFWwindow *WorldSystem::create_window() {
    glfwSetErrorCallback(glfw_err_cb);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 0);
    glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);

    window = glfwCreateWindow(window_width_px, window_height_px, "Orbital Strike", nullptr, nullptr);
    if (window == nullptr) {
        fprintf(stderr, "Failed to glfwCreateWindow");
        return nullptr;
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Failed to initialize SDL Audio");
        return nullptr;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        fprintf(stderr, "Failed to open audio device");
        return nullptr;
    }

    background_music = Mix_LoadMUS(audio_path("music.wav").c_str());
    missile_fire_sound = Mix_LoadWAV(audio_path("missile_fire.wav").c_str());
    missile_destroyed_sound = Mix_LoadWAV(audio_path("missile_destroyed.wav").c_str());
    game_over_sound = Mix_LoadWAV(audio_path("game_over.wav").c_str());

    if (background_music == nullptr || missile_fire_sound == nullptr || missile_destroyed_sound == nullptr ||
        game_over_sound == nullptr) {
        fprintf(stderr, "Failed to load sounds\n %s\n make sure the data directory is present",
                audio_path("music.wav").c_str(),
                audio_path("missile_fire.wav").c_str(),
                audio_path("missile_destroyed.wav").c_str(),
                audio_path("game_over.wav").c_str());
        return nullptr;
    }

    return window;
}

void WorldSystem::init() {
    Mix_PlayMusic(background_music, -1);
    fprintf(stderr, "Loaded music\n");

    Entity p;
    registry.phases.emplace(p);

    callback_system.add_keybind(GLFW_KEY_R, [](GLFWwindow *) { world_system.restart_game(); });

    int all_but_game = (2 * WorldPhase::END - 1) & ~WorldPhase::GAME;
    callback_system.add_keybind(GLFW_KEY_SPACE, [](GLFWwindow *) { world_system.shift_phase(); }, all_but_game);
    callback_system.add_keybind(GLFW_KEY_1, [](GLFWwindow *) { world_system.change_weapon(VARIANT::STANDARD); });
    callback_system.add_keybind(GLFW_KEY_2, [](GLFWwindow *) { world_system.change_weapon(VARIANT::CLUSTER); });
    callback_system.add_keybind(GLFW_KEY_3, [](GLFWwindow *) { world_system.change_weapon(VARIANT::FAST); });
    callback_system.add_keybind(GLFW_KEY_4, [](GLFWwindow *) { world_system.change_weapon(VARIANT::GRAVITY); });
    callback_system.add_mousebind(GLFW_MOUSE_BUTTON_LEFT, [](GLFWwindow *) {
        world_system.spawn_missile_on_mouse(world_system.selected_missile_type);
    });

    callback_system.add_keybind(GLFW_KEY_D, GLFW_PRESS, 0, [](GLFWwindow *) { debugging.in_debug_mode = true; });
    callback_system.add_keybind(GLFW_KEY_D, GLFW_RELEASE, 0, [](GLFWwindow *) { debugging.in_debug_mode = false; });

    callback_system.add_keybind(GLFW_KEY_COMMA, GLFW_RELEASE, GLFW_MOD_SHIFT,
                                [](GLFWwindow *) { world_system.change_speed(-0.1f); });
    callback_system.add_keybind(GLFW_KEY_PERIOD, GLFW_RELEASE, GLFW_MOD_SHIFT,
                                [](GLFWwindow *) { world_system.change_speed(0.1f); });

    callback_system.add_on_mouse_move_callback([](GLFWwindow *w, double, double) { world_system.on_mouse_move(w); });

    restart_game();
}

void WorldSystem::on_mouse_move(GLFWwindow *) {

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    Phase &phase = registry.phases.components[0];

    Entity player_planet = registry.planets.entities[phase.player];
    Motion planet_motion = registry.motions.get(player_planet);

    vec2 pos = camera_system.get_mouse_on_camera(window);
    vec2 mouse_pos_on_camera = vec2(2.f * pos.x - 1.f, 1.f - 2.f * pos.y);

    mat3 proj_matrix = inverse(camera_system.get_projection_matrix(false));

    const vec2 mouse_on_world = proj_matrix * glm::vec3(mouse_pos_on_camera, 1.0f);

    vec2 direction = normalize(mouse_on_world - planet_motion.position);

    Motion &aim_motion = registry.motions.get(aimer);
    aim_motion.position = planet_motion.position + (100.0f * direction);
    aim_motion.angle = std::atan2(direction.y, direction.x);
}

void setPlayersSimulation(bool phase) {
    for (const auto &ent: registry.players.entities) {
        Phase &p = registry.phases.get(ent);
        p.simulation = phase;
    }
}

bool WorldSystem::step(float timeSpentFromLastUpdate) {

    while (registry.debugComponents.entities.size() > 0)
        registry.remove_all_components_of(registry.debugComponents.entities.back());

    auto &motion_container = registry.motions;

    float leftBoundary = -scene_width_px / 2.f;
    float rightBoundary = -leftBoundary;
    float topBoundary = -scene_height_px / 2.f;
    float bottomBoundary = -topBoundary;

    for (int i = 0; i < registry.asteroids.components.size(); i++) {
        Motion &motion = registry.motions.get(registry.asteroids.entities[i]);

        Motion &sun_motion = registry.motions.get(registry.suns.entities[0]);
        float distance = pow(pow(sun_motion.position.x - motion.position.x, 2) +
                             pow(sun_motion.position.y - motion.position.y, 2),
                             0.5);
        if (distance < 0.4 * motion.scale.x) {
            Mix_PlayChannel(-1, world_system.missile_destroyed_sound, 0);
            registry.remove_all_components_of(registry.asteroids.entities[i]);
        }

        for (int j = 0; j < registry.missiles.components.size(); j++) {
            Motion &missile_motion = registry.motions.get(registry.missiles.entities[j]);
            float distance = pow(pow(missile_motion.position.x - motion.position.x, 2) +
                                 pow(missile_motion.position.y - motion.position.y, 2),
                                 0.5);
            if (distance < 0.4 * motion.scale.x) {
                registry.remove_all_components_of(registry.asteroids.entities[i]);
                registry.remove_all_components_of(registry.missiles.entities[j]);
                Mix_PlayChannel(-1, world_system.missile_destroyed_sound, 0);
            }
        }

        for (int j = 0; j < registry.planets.components.size(); j++) {
            Motion &planet_motion = registry.motions.get(registry.planets.entities[j]);
            float distance = pow(pow(planet_motion.position.x - motion.position.x, 2) +
                                 pow(planet_motion.position.y - motion.position.y, 2),
                                 0.5);
            if (distance < 0.4 * motion.scale.x) {
                Mix_PlayChannel(-1, world_system.game_over_sound, 0);
                registry.remove_all_components_of(registry.asteroids.entities[i]);
            }
        }
    }

    for (int i = (int) motion_container.components.size() - 1; i >= 0; --i) {
        Motion &motion = motion_container.components[i];
        float entityLeft = motion.position.x - abs(motion.scale.x);
        float entityRight = motion.position.x + abs(motion.scale.x);
        float entityTop = motion.position.y - abs(motion.scale.y);
        float entityBottom = motion.position.y + abs(motion.scale.y);
        if (registry.missiles.has(motion_container.entities[i])) {
            createParticle(motion.position - abs(motion.scale.x) * normalize(motion.velocity) / 2.f, motion.scale.y);
            if (entityLeft > rightBoundary || entityRight < leftBoundary ||
                entityTop > bottomBoundary || entityBottom < topBoundary) {
                registry.remove_all_components_of(motion_container.entities[i]);
            }
        }
    }

    Phase &p = registry.phases.components[0];
    if (p.simulation) {
        Motion &aim = registry.motions.get(aimer);
        aim.position = vec2(20000.0f, 20000.0f);
    }
    if (p.simulation) {
        for (int i = 0; i < (int) registry.speed_up.components.size(); i++) {
            auto &speed = registry.speed_up.components[i];
            speed.boost = speed.boost - speed.decay_factor;
            speed.ms -= timeSpentFromLastUpdate;

            if (speed.ms < 0 || speed.boost < 1.f) {
                registry.speed_up.remove(registry.speed_up.entities[i]);
            }
        }
    }

    for (int i = 0; i < (int) registry.timers.components.size(); i++) {
        auto &timer = registry.timers.components[i];
        if (timer.simulation && !p.simulation)
            continue;
        Entity entity = registry.timers.entities[i];
        timer.ms -= timeSpentFromLastUpdate;
        if (timer.ms < 0) {
            if (registry.phases.has(entity)) {
                shift_stage();
            }
            if (registry.asteroids.has(entity) && registry.asteroids.components.size() < MAX_ASTEROIDS) {
                createAsteroid(uniform_dist, rng);
            }

            if (timer.death)
                registry.remove_all_components_of(entity);
            else
                registry.timers.remove(entity);
        }
    }

    for (int i = 0; i < registry.planets.components.size(); i++) {
        Planet &p = registry.planets.components[i];

        ImColor bg_color = {0.5f, 0.5f, 0.8f, 1.f};
        float life_duration = 2000.f;
        float blink_duration = life_duration / 5;
        if (p.old_life == p.life) continue;
        p.time_since_change += timeSpentFromLastUpdate;
        if (p.time_since_change > life_duration) {
            p.time_since_change = 0.f;
            p.old_life = p.life;
        }
    }

    return true;
}

void WorldSystem::restart_game() {

    registry.list_all_components();
    printf("Restarting\n");

    current_speed = 1.f;

    while (registry.motions.entities.size() > 0)
        registry.remove_all_components_of(registry.motions.entities.back());
    while (registry.huds.entities.size() > 0)
        registry.remove_all_components_of(registry.motions.entities.back());
    while (registry.timers.entities.size() > 0) {
        Entity &e = registry.timers.entities.back();
        Timer &t = registry.timers.components.back();
        if (t.death)
            registry.remove_all_components_of(e);
        else
            registry.timers.remove(e);
    }
    Phase &phase = registry.phases.components[0];
    phase.simulation = false;
    phase.player = 0;

    registry.list_all_components();


    createBackground();
    createSun({0.f, 0.f}, 100.f, 1000);
    vec2 worm1_pos = vec2(-1500 + 3000 * uniform_dist(rng), -1500 + 3000 * uniform_dist(rng));
    while ((pow(dot(worm1_pos, worm1_pos), 0.5) <= 500)) {
        worm1_pos = vec2(-1500 + 3000 * uniform_dist(rng), -1500 + 3000 * uniform_dist(rng));
    }
    vec2 worm2_pos = vec2(-1500 + 3000 * uniform_dist(rng), -1500 + 3000 * uniform_dist(rng));
    while ((pow(dot(worm2_pos, worm2_pos), 0.5) <= 500
            || abs(pow(dot(worm1_pos, worm1_pos), 0.5) - pow(dot(worm2_pos, worm2_pos), 0.5)) <= 200)) {
        worm2_pos = vec2(-1500 + 3000 * uniform_dist(rng), -1500 + 3000 * uniform_dist(rng));
    }


    worm1 = createWormhole(worm1_pos, 200.f, 0.f);
    worm2 = createWormhole(worm2_pos, 200.f, 0.f);

    vec2 planet1_pos = vec2(-1500 + 3000 * uniform_dist(rng), -1500 + 3000 * uniform_dist(rng));
    while ((pow(dot(planet1_pos, planet1_pos), 0.5) <= 500)) {
        planet1_pos = vec2(-1500 + 3000 * uniform_dist(rng), -1500 + 3000 * uniform_dist(rng));
    }
    vec2 planet2_pos = vec2(-1500 + 3000 * uniform_dist(rng), -1500 + 3000 * uniform_dist(rng));
    while ((pow(dot(planet2_pos, planet2_pos), 0.5) <= 500 ||
            abs(pow(dot(planet1_pos, planet1_pos), 0.5) - pow(dot(planet2_pos, planet2_pos), 0.5)) <= 200)) {
        planet2_pos = vec2(-1500 + 3000 * uniform_dist(rng), -1500 + 3000 * uniform_dist(rng));
    }

    Entity planet1 = createPlanet(planet1_pos, 50.f, 500.f, {0.f, 0.f, 1.f}, M_PI * (1.f + uniform_dist(rng)) / 12.f);
    createPlanet(planet2_pos, 50.f, 500.f, {0.f, 1.f, 0.f}, M_PI * (1.f + uniform_dist(rng)) / 12.f);
    camera_system.lock_on(planet1);

    createAsteroid(uniform_dist, rng);

    aimer = createAimer({200.f, 0.f}, {200.f, 200.f});
    vec2 size = camera_system.camera_size;

    for (int i = 0; i < 4; i++) {
        TEXTURE_ASSET_ID texture = TEXTURE_ASSET_ID::KEY1;
        GEOMETRY_BUFFER_ID geometry = GEOMETRY_BUFFER_ID::MISSILE;
        if (i == 1) {
            texture = TEXTURE_ASSET_ID::KEY2;
            geometry = GEOMETRY_BUFFER_ID::CLUSTER_MISSILE;
        } else if (i == 2) {
            texture = TEXTURE_ASSET_ID::KEY3;
            geometry = GEOMETRY_BUFFER_ID::FAST_MISSILE;
        } else if (i == 3) {
            texture = TEXTURE_ASSET_ID::KEY4;
            geometry = GEOMETRY_BUFFER_ID::GRAVITY_MISSILE;
        }

        float xpos = (i + 1) * 100.f - size.x / 2.f;
        float ypos = 50.f - size.y / 2.f;

        int phases = WorldPhase::GAME | WorldPhase::TUT2;
        createHUDComponent({xpos, ypos}, {30.f, 30.f}, texture, phases);
        auto missile_hud = createHUDComponent({xpos, 100.f + ypos}, {90.f, 30.f},
                                              {TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::MISSILE, geometry},
                                              phases);
        Motion &motion = registry.motions.get(missile_hud);
        motion.angle = M_PI / 2.f;
    }
    vec2 highlight_pos = {100.f - size.x / 2.f, 50.f - size.y / 2.f};
    highlight = createHUDComponent(highlight_pos, {30.f, 30.f}, TEXTURE_ASSET_ID::HIGHLIGHT);
}

void WorldSystem::handle_collisions() {
    auto &collisionsRegistry = registry.collisions;
    std::vector<Entity> marked_entities;
    for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
        Entity entity = collisionsRegistry.entities[i];
        Entity entity_other = collisionsRegistry.components[i].other_entity;

        if (registry.missiles.has(entity)) {
            if (registry.wormholes.has(entity_other)) {
                Motion &motion = registry.motions.get(entity);
                if (entity_other == worm1) {
                    Motion &motion_2 = registry.motions.get(worm2);
                    float el = 0.2;

                    motion.position = motion_2.position + 101.f * normalize(motion.velocity);
                } else if (entity_other == worm2) {
                    Motion &motion_2 = registry.motions.get(worm1);
                    float el = 0.2;
                    motion.position = motion_2.position + 101.f * normalize(motion.velocity);
                }
            } else {
                if (registry.planets.has(entity_other)) {
                    Planet &p = registry.planets.get(entity_other);
                    Missile &m = registry.missiles.get(entity);
                    p.old_life = p.life;
                    p.time_since_change = 0.f;
                    p.life -= m.damage;
                    if (p.life < 0) {
                        Animation anime;
                        anime.does_loop = true;
                        anime.frame_duration = 100.f;
                        anime.nx_frame = 8.f;
                        anime.ny_frame = 8.f;

                        render_system.changeAnimation(entity_other, anime);
                    }
                }
                marked_entities.push_back(entity);
            }


        }
    }
    for (Entity e: marked_entities) {
        registry.remove_all_components_of(e);
    }
    registry.collisions.clear();
}

bool WorldSystem::is_over() const {
    return bool(glfwWindowShouldClose(window));
}

void WorldSystem::spawn_missile_on_mouse(VARIANT m_type) {
    Phase &phase = registry.phases.components[0];
    if (phase.simulation)
        return;
    shift_stage();

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    Entity player_planet = registry.planets.entities[phase.player];
    Motion planet_motion = registry.motions.get(player_planet);
    vec2 pos = camera_system.get_mouse_on_camera(window);
    vec2 mouse_pos_on_camera = vec2(2.f * pos.x - 1.f, 1.f - 2.f * pos.y);
    mat3 proj_matrix = inverse(camera_system.get_projection_matrix(false));
    const vec2 mouse_on_world = proj_matrix * glm::vec3(mouse_pos_on_camera, 1.0f);
    vec2 direction = normalize(mouse_on_world - planet_motion.position);
    createMissile(planet_motion, direction, m_type);
}

void WorldSystem::change_weapon(VARIANT type) {
    vec2 size = camera_system.camera_size;
    Motion &motion = registry.motions.get(highlight);
    motion.position = {type * (100.f) - size.x / 2.f, 50.f - size.y / 2.f};
    selected_missile_type = type;

    RenderRequest &rr = registry.renderRequests.get(aimer);
    switch (type) {
        case (VARIANT::STANDARD):
            rr.used_geometry = GEOMETRY_BUFFER_ID::FAST_MISSILE;
            break;
        case (VARIANT::CLUSTER):
            rr.used_geometry = GEOMETRY_BUFFER_ID::CLUSTER_MISSILE;
            break;

        case (VARIANT::FAST):
            rr.used_geometry = GEOMETRY_BUFFER_ID::FAST_MISSILE;
            break;

        case (VARIANT::GRAVITY):
            rr.used_geometry = GEOMETRY_BUFFER_ID::GRAVITY_MISSILE;
            break;
        default:
            rr.used_geometry = GEOMETRY_BUFFER_ID::MISSILE;
    }
}

void WorldSystem::change_speed(float delta) {
    current_speed = fmax(0.f, current_speed + delta);
    printf("Current speed = %f\n", current_speed);
}

void WorldSystem::shift_stage() {
    Phase &phase = registry.phases.components[0];
    if (!phase.simulation) {
        Timer &t = registry.timers.emplace(registry.phases.entities[0]);
        t.ms = 5000.f;
    } else {
        phase.player = (1 + phase.player) % 2;
        Entity p = registry.planets.entities[phase.player];
        camera_system.lock_on(p);
    }
    phase.simulation = !phase.simulation;
}

void WorldSystem::shift_phase() {
    Phase &phase = registry.phases.components[0];
    if (phase.phase == WorldPhase::GAME) {
        Entity e = registry.phases.entities[0];
        if (registry.timers.has(e)) registry.timers.remove(e);
        phase.simulation = false;
        for (int i = 0; i < registry.planets.components.size(); i++)
            if (registry.planets.components[i].life > 0.f) {
                camera_system.lock_on(registry.planets.entities[i]);
                break;
            }
    }
    if (phase.phase == WorldPhase::END) {
        phase.phase = WorldPhase::GAME;
        restart_game();
    } else phase.phase = static_cast<WorldPhase>(2 * phase.phase);
}