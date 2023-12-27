#include "callback_system.hpp"
#include "tiny_ecs_registry.hpp"

CallbackSystem callback_system;

void CallbackSystem::init(GLFWwindow *window) {
    glfwSetKeyCallback(window, [](GLFWwindow *wnd, int _0, int, int _1, int _2) {
        callback_system.on_key(wnd, _0, _1, _2);
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *wnd, int _0, int _1, int _2) {
        callback_system.on_mouse_button(wnd, _0, _1, _2);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow *wnd, double _0, double _1) {
        callback_system.on_mouse_move(wnd, _0, _1);
    });
    glfwSetScrollCallback(window, [](GLFWwindow *wnd, double _0, double _1) {
        callback_system.on_scroll(wnd, _0, _1);
    });

}


void CallbackSystem::add_bind(bool is_key, int key, int action, int mods, VOID_FUNC func, int phases) {
    Keybind k;
    k.action = action;
    k.mods = mods;
    k.func = func;
    k.phases = phases;
    std::vector<Keybind> v;
    std::unordered_map<int, std::vector<Keybind>> &bind_map = is_key ? key_binds : mouse_binds;
    bind_map.insert({key, v});
    bind_map[key].push_back(k);
}

void CallbackSystem::add_keybind(int key, int action, int mods, VOID_FUNC func, int phases) {
    add_bind(true, key, action, mods, func, phases);
}

void CallbackSystem::add_keybind(int key, VOID_FUNC func, int phases) {
    add_keybind(key, GLFW_RELEASE, 0, func, phases);
}

void CallbackSystem::add_mousebind(int key, int action, int mods, VOID_FUNC func, int phases) {
    add_bind(false, key, action, mods, func, phases);
}

void CallbackSystem::add_mousebind(int key, VOID_FUNC func, int phases) {
    add_mousebind(key, GLFW_RELEASE, 0, func, phases);
}

void CallbackSystem::add_on_mouse_move_callback(GLFWcursorposfun func, int phases) {
    Callback<GLFWcursorposfun> cb = {phases, func};
    on_mouse_move_callbacks.push_back(cb);
}

void CallbackSystem::add_on_scroll_callback(GLFWscrollfun func, int phases) {
    Callback<GLFWscrollfun> cb = {phases, func};
    on_scroll_callbacks.push_back(cb);
}

void on_map(std::unordered_map<int, std::vector<Keybind>> &map, GLFWwindow *wnd, int key, int action, int mods) {
    WorldPhase phase = registry.phases.components[0].phase;
    if (map.find(key) == map.end()) return;
    for (Keybind k: map[key])
        if ((k.phases & phase) == phase && k.action == action && k.mods == mods) k.func(wnd);
}

void CallbackSystem::on_key(GLFWwindow *wnd, int key, int action, int mods) {
    on_map(key_binds, wnd, key, action, mods);
}

void CallbackSystem::on_mouse_button(GLFWwindow *wnd, int key, int action, int mods) {
    on_map(mouse_binds, wnd, key, action, mods);
}

template<typename T>
void on_vec(std::vector<Callback<T>> &v, GLFWwindow *wnd, double x, double y) {
    WorldPhase phase = registry.phases.components[0].phase;
    for (auto cb: v)
        if ((cb.phases & phase) == phase) cb.func(wnd, x, y);
}


void CallbackSystem::on_mouse_move(GLFWwindow *wnd, double x, double y) {
    on_vec<GLFWcursorposfun>(on_mouse_move_callbacks, wnd, x, y);
}

void CallbackSystem::on_scroll(GLFWwindow *wnd, double xoffset, double yoffset) {
    on_vec<GLFWscrollfun>(on_scroll_callbacks, wnd, xoffset, yoffset);
}