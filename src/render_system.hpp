#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"

struct TextHeader {
    const char *text;
    bool is_header;
};

class RenderSystem {
    std::array<GLuint, texture_count> texture_gl_handles;
    std::array<ivec2, texture_count> texture_dimensions;
    const std::vector<std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths =
            {};

    const std::array<std::string, texture_count> texture_paths = {
            textures_path("ice.png"),
            textures_path("sun.png"),
            textures_path("key_1.png"),
            textures_path("key_2.png"),
            textures_path("key_3.png"),
            textures_path("key_4.png"),
            textures_path("asteroid.png"),
            textures_path("asteroid2.png"),
            textures_path("asteroid3.png"),
            textures_path("background.jpg"),
            textures_path("wormhole.png"),
            textures_path("highlight.png"),
    };


    std::array<GLuint, effect_count> effects;
    const std::array<std::string, effect_count> effect_paths = {
            shader_path("colored"),
            shader_path("pebble"),
            shader_path("missile"),
            shader_path("smoke_particle"),
            shader_path("textured"),
            shader_path("animated"),
            shader_path("space")};

    std::array<GLuint, geometry_count> vertex_buffers;
    std::array<GLuint, geometry_count> index_buffers;
    std::array<Mesh, geometry_count> meshes;

public:
    bool init(GLFWwindow *window);

    template<class T>
    void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

    void initializeGlTextures();

    void initializeGlEffects();

    void initializeGlMeshes();

    Mesh &getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int) id]; };

    void initializeGlGeometryBuffers();

    bool initScreenTexture();

    ~RenderSystem();

    void draw(float elapsed_ms);

    void changeAnimation(Entity entity, Animation anime);

private:
    ImVec2 imguize(vec2 vector);

    void drawMainMenu();

    void drawGameHud();

    void updateVisibilityHudEntities();

    void drawGameOver();

    void drawHUD();

    void centreText(ImDrawList *draw_list, const char *text, vec2 cursor_pos, bool is_header = false);

    template<size_t N>
    void centreText(ImDrawList *draw_list, TextHeader (&text_arr)[N], vec2 cursor_pos);

    void drawRectangle(ImDrawList *draw_list, vec2 position, vec2 size, vec4 color, vec2 padding = {0.f, 0.f});

    void startDummyWindow(const char *name, vec2 position, vec2 size, float alpha = 0.f);

    void drawTexturedMesh(Entity entity, const mat3 &projection, float elapsed_ms);

    void drawToScreen();

    GLFWwindow *window;

    GLuint frame_buffer;
    GLuint off_screen_render_buffer_color;
    GLuint off_screen_render_buffer_depth;

    Entity screen_state_entity;

    ImFont *header;

    float imgui_scale;
};

extern RenderSystem render_system;

bool loadEffectFromFile(
        const std::string &vs_path, const std::string &fs_path, GLuint &out_program);

void normalize_meshes(Mesh &mesh);