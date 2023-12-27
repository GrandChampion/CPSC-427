#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"


typedef void (*VOID_FUNC)(GLFWwindow *window);

struct Keybind {
    int phases = WorldPhase::GAME;
    int action = GLFW_KEY_0;
    int mods = 0;
    VOID_FUNC func = [](GLFWwindow *) {};
};

template<typename T>
struct Callback {
    int phases = WorldPhase::GAME;
    T func;
};


class CallbackSystem {
public:
    void init(GLFWwindow *window);

    void add_bind(bool iskey, int key, int action, int mods, VOID_FUNC func, int phases);

    void add_keybind(int key, int action, int mods, VOID_FUNC func, int phases = WorldPhase::GAME);

    void add_keybind(int key, VOID_FUNC func, int phases = WorldPhase::GAME);

    void add_mousebind(int key, int action, int mods, VOID_FUNC func, int phases = WorldPhase::GAME);

    void add_mousebind(int key, VOID_FUNC func, int phases = WorldPhase::GAME);

    void add_on_mouse_move_callback(GLFWcursorposfun func, int phases = WorldPhase::GAME);

    void add_on_scroll_callback(GLFWscrollfun func, int phases = WorldPhase::GAME);

    bool in_hud;
private:
    std::unordered_map<int, std::vector<Keybind>> key_binds;
    std::unordered_map<int, std::vector<Keybind>> mouse_binds;
    std::vector<Callback<GLFWcursorposfun>> on_mouse_move_callbacks;
    std::vector<Callback<GLFWscrollfun>> on_scroll_callbacks;

    void on_key(GLFWwindow *wnd, int key, int action, int mods);

    void on_mouse_button(GLFWwindow *wnd, int key, int action, int mods);

    void on_scroll(GLFWwindow *wnd, double xoffset, double yoffset);

    void on_mouse_move(GLFWwindow *wnd, double x, double y);
};

extern CallbackSystem callback_system;