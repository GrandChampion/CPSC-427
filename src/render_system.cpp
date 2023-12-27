#include "render_system.hpp"
#include <SDL.h>

#include "tiny_ecs_registry.hpp"
#include "camera_system.hpp"

RenderSystem render_system;

void RenderSystem::drawTexturedMesh(Entity entity, const mat3 &projection, float elapsed_ms) {
    if (registry.hidden.has(entity)) return;
    Motion &motion = registry.motions.get(entity);
    Transform transform;
    transform.translate(motion.position);
    transform.rotate(motion.angle);
    transform.scale(motion.scale);

    assert(registry.renderRequests.has(entity));
    const RenderRequest &render_request = registry.renderRequests.get(entity);

    const GLuint used_effect_enum = (GLuint) render_request.used_effect;
    assert(used_effect_enum != (GLuint) EFFECT_ASSET_ID::EFFECT_COUNT);
    const GLuint program = (GLuint) effects[used_effect_enum];

    glUseProgram(program);
    gl_has_errors();

    assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
    const GLuint vbo = vertex_buffers[(GLuint) render_request.used_geometry];
    const GLuint ibo = index_buffers[(GLuint) render_request.used_geometry];

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl_has_errors();

    if (render_request.used_effect == EFFECT_ASSET_ID::TEXTURED ||
        render_request.used_effect == EFFECT_ASSET_ID::ANIMATED) {
        GLint in_position_loc = glGetAttribLocation(program, "in_position");
        GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
        gl_has_errors();
        assert(in_texcoord_loc >= 0);

        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                              sizeof(TexturedVertex), (void *) 0);
        gl_has_errors();

        glEnableVertexAttribArray(in_texcoord_loc);
        glVertexAttribPointer(
                in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
                (void *) sizeof(vec3));

        glActiveTexture(GL_TEXTURE0);
        gl_has_errors();

        assert(registry.renderRequests.has(entity));
        GLuint texture_id = texture_gl_handles[(GLuint) render_request.used_texture];

        if (render_request.used_effect == EFFECT_ASSET_ID::ANIMATED) {
            assert(registry.animations.has(entity));
            Animation &anime = registry.animations.get(entity);
            if (registry.suns.has(entity))
                transform.scale({4.f, 4.f});
            anime.total_elapsed += elapsed_ms;
            if (anime.total_elapsed > anime.nx_frame * anime.ny_frame * anime.frame_duration) {
                anime.total_elapsed = 0.f;
            }

            float uv_x = floor(anime.total_elapsed / anime.frame_duration);
            float uv_y = floor(anime.total_elapsed / (anime.frame_duration * anime.nx_frame));

            glUniform1f(glGetUniformLocation(program, "nx_frames"), anime.nx_frame);
            glUniform1f(glGetUniformLocation(program, "ny_frames"), anime.ny_frame);

            glUniform1f(glGetUniformLocation(program, "uv_x"), uv_x);
            glUniform1f(glGetUniformLocation(program, "uv_y"), uv_y);

            vec3 bcolor = vec3(1);
            float bwidth = 0.f;
            if (registry.planets.has(entity)) {
                Planet &p = registry.planets.get(entity);
                bcolor = p.color;
                bwidth = 0.025f;
            }
            GLint bcolor_uloc = glGetUniformLocation(program, "bcolor");
            glUniform3fv(bcolor_uloc, 1, (float *) &bcolor);
            gl_has_errors();
            GLint bwidth_uloc = glGetUniformLocation(program, "bwidth");
            glUniform1f(bwidth_uloc, bwidth);
            gl_has_errors();
        }

        glBindTexture(GL_TEXTURE_2D, texture_id);
        gl_has_errors();

    } else if (render_request.used_effect == EFFECT_ASSET_ID::MISSILE ||
               render_request.used_effect == EFFECT_ASSET_ID::PEBBLE) {
        GLint in_position_loc = glGetAttribLocation(program, "in_position");
        GLint in_color_loc = glGetAttribLocation(program, "in_color");
        gl_has_errors();

        GLuint time_uloc = glGetUniformLocation(program, "timing");
        glUniform1f(time_uloc, (float) (glfwGetTime() * 10.0f));

        gl_has_errors();

        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                              sizeof(ColoredVertex), (void *) 0);
        gl_has_errors();

        glEnableVertexAttribArray(in_color_loc);
        glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
                              sizeof(ColoredVertex), (void *) sizeof(vec3));
        gl_has_errors();

    } else if (render_request.used_effect == EFFECT_ASSET_ID::SMOKE) {
        GLint in_position_loc = glGetAttribLocation(program, "in_position");
        gl_has_errors();

        GLuint time_uloc = glGetUniformLocation(program, "timing");
        glUniform1f(time_uloc, (float) glfwGetTime());

        gl_has_errors();

        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void *) 0);
        gl_has_errors();
    } else {
        assert(false && "Type of render request not supported");
    }

    vec3 color = vec3(1);
    if (registry.planets.has(entity) &&
        registry.planets.entities[registry.phases.components[0].player] != entity) {
        color *= vec3(0.5f, 0.5f, 0.5f);
    }
    GLint color_uloc = glGetUniformLocation(program, "fcolor");
    glUniform3fv(color_uloc, 1, (float *) &color);
    gl_has_errors();

    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();

    GLsizei num_indices = size / sizeof(uint16_t);

    GLint currProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
    GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
    glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float *) &transform.mat);
    GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *) &projection);
    gl_has_errors();
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    gl_has_errors();
}

void RenderSystem::drawToScreen() {
    glUseProgram(effects[(GLuint) EFFECT_ASSET_ID::WATER]);
    gl_has_errors();
    int w, h;
    glfwGetFramebufferSize(window, &w,
                           &h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glDepthRange(0, 10);
    glClearColor(1.f, 0, 0, 1.0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint) GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
    glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            index_buffers[(GLuint) GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
    gl_has_errors();
    const GLuint water_program = effects[(GLuint) EFFECT_ASSET_ID::WATER];
    GLuint time_uloc = glGetUniformLocation(water_program, "time");
    GLuint dead_timer_uloc = glGetUniformLocation(water_program, "screen_darken_factor");
    glUniform1f(time_uloc, (float) (glfwGetTime() * 10.0f));
    ScreenState &screen = registry.screenStates.get(screen_state_entity);
    glUniform1f(dead_timer_uloc, screen.screen_darken_factor);
    gl_has_errors();
    GLint in_position_loc = glGetAttribLocation(water_program, "in_position");
    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *) 0);
    gl_has_errors();
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
    gl_has_errors();
    glDrawElements(
            GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
            nullptr);
    gl_has_errors();
}

void RenderSystem::draw(float elapsed_ms) {
    int w, h;
    glfwGetFramebufferSize(window, &w,
                           &h);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    gl_has_errors();
    glViewport(0, 0, w, h);
    glDepthRange(0.00001, 10);
    glClearColor(0, 0, 0, 1.0);
    glClearDepth(10.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    gl_has_errors();
    updateVisibilityHudEntities();

    mat3 proj_matrix = camera_system.get_projection_matrix(false);
    mat3 hud_proj_matrix = camera_system.get_projection_matrix(true);
    for (Entity entity: registry.renderRequests.entities) {
        if (!registry.motions.has(entity))
            continue;
        mat3 projection_2D = registry.huds.has(entity) ? hud_proj_matrix : proj_matrix;
        drawTexturedMesh(entity, projection_2D, elapsed_ms);
    }
    drawToScreen();
    drawHUD();
    glfwSwapBuffers(window);
    gl_has_errors();
}

void RenderSystem::changeAnimation(Entity entity, Animation anime) {
    if (registry.animations.has(entity)) {
        registry.renderRequests.remove(entity);
        registry.animations.remove(entity);

        registry.renderRequests.insert(entity,
                                       {
                                               TEXTURE_ASSET_ID::ASTEROID,
                                               EFFECT_ASSET_ID::ANIMATED,
                                               GEOMETRY_BUFFER_ID::SPRITE
                                       });
        registry.animations.insert(entity, anime);
    };


}