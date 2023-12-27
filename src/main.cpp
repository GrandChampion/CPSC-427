
#define GL3W_IMPLEMENTATION

#include <chrono>

#include "physics_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "camera_system.hpp"
#include "callback_system.hpp"

using Clock = std::chrono::high_resolution_clock;

int main() {

    GLFWwindow *window = world_system.create_window();
    if (!window) {
        printf("Press any key to exit");
        getchar();
        return EXIT_FAILURE;
    }
    callback_system.init(window);
    camera_system.init(window);
    render_system.init(window);
    world_system.init();

    auto t = Clock::now();

    while (!world_system.is_over()) {
        glfwPollEvents();
        glfwPollEvents();
        auto now = Clock::now();
        float elapsed_ms = (float) (std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
        t = now;
        world_system.step(elapsed_ms);
        physics_system.step(elapsed_ms);
        world_system.handle_collisions();
        camera_system.step(elapsed_ms);
        render_system.draw(elapsed_ms);


        if (registry.phases.components[0].phase == WorldPhase::GAME)
            for (Planet &p: registry.planets.components)
                if (p.life <= 0) {
                    // play game over sound
                    Mix_PlayChannel(-1, world_system.game_over_sound, 0);
                    world_system.shift_phase();
                    break;
                }
    }

    return EXIT_SUCCESS;
}
