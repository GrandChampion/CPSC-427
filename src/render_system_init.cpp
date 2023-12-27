#include "render_system.hpp"

#include <array>
#include <fstream>

#include "../ext/stb_image/stb_image.h"

#include "tiny_ecs_registry.hpp"
#include "camera_system.hpp"
#include <iostream>
#include <sstream>

bool RenderSystem::init(GLFWwindow *window_arg) {
    this->window = window_arg;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    int w, h;
    glfwGetWindowSize(window, &w, &h);

    imgui_scale = w / camera_system.camera_size.x;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    std::string font = data_path() + "/Montserrat-Bold.ttf";
    io.Fonts->AddFontFromFileTTF(font.c_str(), 32 * imgui_scale);
    header = io.Fonts->AddFontFromFileTTF(font.c_str(), 64 * imgui_scale);;
    ImGui::StyleColorsLight();
    ImGuiStyle *style = &ImGui::GetStyle();
    style->Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    const int is_fine = gl3w_init();
    assert(is_fine == 0);
    frame_buffer = 0;
    glGenFramebuffers(1, &frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    gl_has_errors();

    int frame_buffer_width_px, frame_buffer_height_px;
    glfwGetFramebufferSize(window, &frame_buffer_width_px,
                           &frame_buffer_height_px);
    if (frame_buffer_width_px != w) {
        printf("glfwGetFramebufferSize = %d,%d\n", frame_buffer_width_px, frame_buffer_height_px);
        printf("window width_height = %d,%d\n", w, window_height_px);
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    gl_has_errors();

    initScreenTexture();
    initializeGlTextures();
    initializeGlEffects();
    initializeGlGeometryBuffers();

    return true;
}

void RenderSystem::initializeGlTextures() {
    glGenTextures((GLsizei) texture_gl_handles.size(), texture_gl_handles.data());

    for (uint i = 0; i < texture_paths.size(); i++) {
        const std::string &path = texture_paths[i];
        ivec2 &dimensions = texture_dimensions[i];

        stbi_uc *data;
        data = stbi_load(path.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

        if (data == NULL) {
            const std::string message = "Could not load the file " + path + ".";
            fprintf(stderr, "%s", message.c_str());
            assert(false);
        }
        glBindTexture(GL_TEXTURE_2D, texture_gl_handles[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl_has_errors();
        stbi_image_free(data);
    }
    gl_has_errors();
}

void RenderSystem::initializeGlEffects() {
    for (uint i = 0; i < effect_paths.size(); i++) {
        const std::string vertex_shader_name = effect_paths[i] + ".vs.glsl";
        const std::string fragment_shader_name = effect_paths[i] + ".fs.glsl";

        bool is_valid = loadEffectFromFile(vertex_shader_name, fragment_shader_name, effects[i]);
        assert(is_valid && (GLuint) effects[i] != 0);
    }
}

template<class T>
void RenderSystem::bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices) {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(uint) gid]);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    gl_has_errors();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[(uint) gid]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
    gl_has_errors();
}

void RenderSystem::initializeGlMeshes() {
    for (uint i = 0; i < mesh_paths.size(); i++) {
        GEOMETRY_BUFFER_ID geom_index = mesh_paths[i].first;
        std::string name = mesh_paths[i].second;
        Mesh::loadFromOBJFile(name,
                              meshes[(int) geom_index].vertices,
                              meshes[(int) geom_index].vertex_indices,
                              meshes[(int) geom_index].original_size);

        bindVBOandIBO(geom_index,
                      meshes[(int) geom_index].vertices,
                      meshes[(int) geom_index].vertex_indices);
    }
}

void RenderSystem::initializeGlGeometryBuffers() {
    glGenBuffers((GLsizei) vertex_buffers.size(), vertex_buffers.data());
    glGenBuffers((GLsizei) index_buffers.size(), index_buffers.data());

    initializeGlMeshes();

    std::vector<TexturedVertex> textured_vertices(4);
    textured_vertices[0].position = {-1.f / 2, +1.f / 2, 0.f};
    textured_vertices[1].position = {+1.f / 2, +1.f / 2, 0.f};
    textured_vertices[2].position = {+1.f / 2, -1.f / 2, 0.f};
    textured_vertices[3].position = {-1.f / 2, -1.f / 2, 0.f};
    textured_vertices[0].texcoord = {0.f, 0.f};
    textured_vertices[1].texcoord = {1.f, 0.f};
    textured_vertices[2].texcoord = {1.f, 1.f};
    textured_vertices[3].texcoord = {0.f, 1.f};

    const std::vector<uint16_t> textured_indices = {0, 3, 1, 1, 3, 2};
    bindVBOandIBO(GEOMETRY_BUFFER_ID::SPRITE, textured_vertices, textured_indices);

    std::vector<ColoredVertex> pebble_vertices;
    std::vector<uint16_t> pebble_indices;
    constexpr float z = -0.1f;
    constexpr int NUM_TRIANGLES = 62;

    for (int i = 0; i < NUM_TRIANGLES; i++) {
        const float t = float(i) * M_PI * 2.f / float(NUM_TRIANGLES - 1);
        pebble_vertices.push_back(ColoredVertex({1.0 * cos(t), 1.0 * sin(t), z}, {0.8, 0.8, 0.8}));
    }

    pebble_vertices.push_back(ColoredVertex({0, 0, 0}, {0.8, 0.8, 0.8}));

    for (int i = 0; i < NUM_TRIANGLES; i++) {
        pebble_indices.push_back((uint16_t) i);
        pebble_indices.push_back((uint16_t) ((i + 1) % NUM_TRIANGLES));
        pebble_indices.push_back((uint16_t) NUM_TRIANGLES);
    }

    int geom_index = (int) GEOMETRY_BUFFER_ID::PEBBLE;
    meshes[geom_index].vertices = pebble_vertices;
    meshes[geom_index].vertex_indices = pebble_indices;
    bindVBOandIBO(GEOMETRY_BUFFER_ID::PEBBLE, meshes[geom_index].vertices, meshes[geom_index].vertex_indices);


    std::vector<vec2> positions = {
            {-1.5f, 0.25f},
            {-1.5f, -0.25f},
            {-0.5f, 0.f},
            {-0.5f, 0.5f},
            {-0.5f, -0.5f},
            {0.5f,  0.5f},
            {0.5f,  -0.5f},
            {1.5f,  0.f},
    };
    for (int j = 0; j < 4; j++) {
        std::vector<ColoredVertex> missileVertexList;
        std::vector<uint16_t> missilePointIndexList;
        GEOMETRY_BUFFER_ID index;
        vec3 missileColor;
        if (j == 0) {
            index = GEOMETRY_BUFFER_ID::MISSILE;
            missileColor = {0, 0.38, 0.21};
        } else if (j == 1) {
            index = GEOMETRY_BUFFER_ID::FAST_MISSILE;
            missileColor = {0.5, 0.21, 0.21};
        } else if (j == 2) {
            index = GEOMETRY_BUFFER_ID::CLUSTER_MISSILE;
            missileColor = {0.21, 0.21, 0.5};
        } else {
            index = GEOMETRY_BUFFER_ID::GRAVITY_MISSILE;
            missileColor = {0.21, 0.21, 0.21};
        }

        {
            uint16_t i = 0;
            for (vec2 pos: positions) {
                missileVertexList.push_back(ColoredVertex(vec3(pos, 0), missileColor));
                missilePointIndexList.push_back(i);
                if (i == 4) {
                    missilePointIndexList.push_back((uint16_t) 6);
                    missilePointIndexList.push_back((uint16_t) 3);
                    missilePointIndexList.push_back((uint16_t) 6);
                    missilePointIndexList.push_back((uint16_t) 5);
                }
                i++;
            }
        }

        int geometryIndex = (int) index;
        meshes[geometryIndex].vertices = missileVertexList;
        meshes[geometryIndex].vertex_indices = missilePointIndexList;
        meshes[geometryIndex].original_size = {3.f, 1.f};
        normalize_meshes(meshes[geometryIndex]);
        bindVBOandIBO(index, meshes[geometryIndex].vertices, meshes[geometryIndex].vertex_indices);
    }


    std::vector<ColoredVertex> smokeVertexList;
    std::vector<uint16_t> smokePointIndexList;

    vec3 smokeColor = {1, 0, 0};

    int howManyTriangleInSmoke = 30;
    float radius = 1.5f;
    for (int i = 0; i < howManyTriangleInSmoke; i++) {
        const float t = float(i) * M_PI * 2.f / float(howManyTriangleInSmoke - 1);
        smokeVertexList.push_back(ColoredVertex({1.0 * cos(t), 1.0 * sin(t), -1 * radius}, smokeColor));
    }

    smokeVertexList.push_back(ColoredVertex({0, 0, 0}, smokeColor));

    for (int i = 0; i < NUM_TRIANGLES; i++) {
        smokePointIndexList.push_back((uint16_t) i);
        smokePointIndexList.push_back((uint16_t) ((i + 1) % howManyTriangleInSmoke));
        smokePointIndexList.push_back((uint16_t) howManyTriangleInSmoke);
    }

    int smokeGeometryBuffer = (int) GEOMETRY_BUFFER_ID::SMOKE;
    meshes[smokeGeometryBuffer].vertices = smokeVertexList;
    meshes[smokeGeometryBuffer].vertex_indices = smokePointIndexList;
    bindVBOandIBO(GEOMETRY_BUFFER_ID::SMOKE, meshes[smokeGeometryBuffer].vertices,
                  meshes[smokeGeometryBuffer].vertex_indices);


    std::vector<ColoredVertex> line_vertices;
    std::vector<uint16_t> line_indices;

    constexpr float depth = 0.5f;
    constexpr vec3 red = {0.8, 0.1, 0.1};

    line_vertices = {
            {{-0.5, -0.5, depth}, red},
            {{-0.5, 0.5,  depth}, red},
            {{0.5,  0.5,  depth}, red},
            {{0.5,  -0.5, depth}, red},
    };

    line_indices = {0,
                    1,
                    3,
                    1,
                    2,
                    3};

    geom_index = (int) GEOMETRY_BUFFER_ID::DEBUG_LINE;
    meshes[geom_index].vertices = line_vertices;
    meshes[geom_index].vertex_indices = line_indices;
    bindVBOandIBO(GEOMETRY_BUFFER_ID::DEBUG_LINE, line_vertices, line_indices);

    std::vector<vec3> screen_vertices(3);
    screen_vertices[0] = {-1, -6, 0.f};
    screen_vertices[1] = {6, -1, 0.f};
    screen_vertices[2] = {-1, 6, 0.f};

    const std::vector<uint16_t> screen_indices = {0, 1, 2};
    bindVBOandIBO(GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE, screen_vertices, screen_indices);
}

RenderSystem::~RenderSystem() {
    glDeleteBuffers((GLsizei) vertex_buffers.size(), vertex_buffers.data());
    glDeleteBuffers((GLsizei) index_buffers.size(), index_buffers.data());
    glDeleteTextures((GLsizei) texture_gl_handles.size(), texture_gl_handles.data());
    glDeleteTextures(1, &off_screen_render_buffer_color);
    glDeleteRenderbuffers(1, &off_screen_render_buffer_depth);
    gl_has_errors();

    for (uint i = 0; i < effect_count; i++) {
        glDeleteProgram(effects[i]);
    }
    glDeleteFramebuffers(1, &frame_buffer);
    gl_has_errors();

    while (registry.renderRequests.entities.size() > 0)
        registry.remove_all_components_of(registry.renderRequests.entities.back());

    glfwDestroyWindow(window);
}

bool RenderSystem::initScreenTexture() {
    registry.screenStates.emplace(screen_state_entity);

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(const_cast<GLFWwindow *>(window), &framebuffer_width,
                           &framebuffer_height);

    glGenTextures(1, &off_screen_render_buffer_color);
    glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl_has_errors();

    glGenRenderbuffers(1, &off_screen_render_buffer_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, off_screen_render_buffer_depth);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, off_screen_render_buffer_color, 0);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, framebuffer_width, framebuffer_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, off_screen_render_buffer_depth);
    gl_has_errors();

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    return true;
}

bool gl_compile_shader(GLuint shader) {
    glCompileShader(shader);
    gl_has_errors();
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        std::vector<char> log(log_len);
        glGetShaderInfoLog(shader, log_len, &log_len, log.data());
        glDeleteShader(shader);

        gl_has_errors();

        fprintf(stderr, "GLSL: %s", log.data());
        return false;
    }

    return true;
}

bool loadEffectFromFile(
        const std::string &vs_path, const std::string &fs_path, GLuint &out_program) {
    std::ifstream vs_is(vs_path);
    std::ifstream fs_is(fs_path);
    if (!vs_is.good() || !fs_is.good()) {
        fprintf(stderr, "Failed to load shader files %s, %s", vs_path.c_str(), fs_path.c_str());
        assert(false);
        return false;
    }

    std::stringstream vs_ss, fs_ss;
    vs_ss << vs_is.rdbuf();
    fs_ss << fs_is.rdbuf();
    std::string vs_str = vs_ss.str();
    std::string fs_str = fs_ss.str();
    const char *vs_src = vs_str.c_str();
    const char *fs_src = fs_str.c_str();
    GLsizei vs_len = (GLsizei) vs_str.size();
    GLsizei fs_len = (GLsizei) fs_str.size();

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vs_src, &vs_len);
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fs_src, &fs_len);
    gl_has_errors();

    if (!gl_compile_shader(vertex)) {
        fprintf(stderr, "Vertex compilation failed");
        assert(false);
        return false;
    }
    if (!gl_compile_shader(fragment)) {
        fprintf(stderr, "Vertex compilation failed");
        assert(false);
        return false;
    }

    out_program = glCreateProgram();
    glAttachShader(out_program, vertex);
    glAttachShader(out_program, fragment);
    glLinkProgram(out_program);
    gl_has_errors();

    {
        GLint is_linked = GL_FALSE;
        glGetProgramiv(out_program, GL_LINK_STATUS, &is_linked);
        if (is_linked == GL_FALSE) {
            GLint log_len;
            glGetProgramiv(out_program, GL_INFO_LOG_LENGTH, &log_len);
            std::vector<char> log(log_len);
            glGetProgramInfoLog(out_program, log_len, &log_len, log.data());
            gl_has_errors();

            fprintf(stderr, "Link error: %s", log.data());
            assert(false);
            return false;
        }
    }

    glDetachShader(out_program, vertex);
    glDetachShader(out_program, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    gl_has_errors();

    return true;
}

void normalize_meshes(Mesh &mesh) {
    vec3 max_position = {-99999, -99999, -99999};
    vec3 min_position = {99999, 99999, 99999};
    for (ColoredVertex &pos: mesh.vertices) {
        max_position = glm::max(max_position, pos.position);
        min_position = glm::min(min_position, pos.position);
    }
    if (abs(max_position.z - min_position.z) < 0.001)
        max_position.z = min_position.z + 1;

    vec3 size3d = max_position - min_position;
    mesh.original_size = size3d;

    for (ColoredVertex &pos: mesh.vertices)
        pos.position = ((pos.position - min_position) / size3d) - vec3(0.5f, 0.5f, 0.5f);
}
