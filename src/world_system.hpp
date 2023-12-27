#pragma once

#include "common.hpp"
#include "components.hpp"

#include <vector>
#include <random>
#include <list>

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_mixer.h>


class WorldSystem {
public:
    WorldSystem();

    GLFWwindow *create_window();

    void init();

    ~WorldSystem();

    bool step(float timeSpentFromLastUpdate);

    void handle_collisions();

    bool is_over() const;

    std::list<Entity> action_list;

    void shift_stage();

    void restart_game();

    void spawn_missile_on_mouse(VARIANT m_type);

    void change_speed(float delta);

    void on_mouse_move(GLFWwindow *w);

    void change_weapon(VARIANT type);

    VARIANT selected_missile_type = VARIANT::STANDARD;

    void shift_phase();

    Mix_Music *background_music;
    Mix_Chunk *missile_fire_sound;
    Mix_Chunk *missile_destroyed_sound;
    Mix_Chunk *game_over_sound;

private:

    GLFWwindow *window;

    float current_speed;

    Entity aimer;

    Entity worm1;
    Entity worm2;
    Entity highlight;
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist;
};

extern WorldSystem world_system;