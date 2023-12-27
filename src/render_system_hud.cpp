#include "render_system.hpp"
#include "camera_system.hpp"
#include "world_system.hpp"
#include "callback_system.hpp"
#include "tiny_ecs_registry.hpp"

ImVec2 RenderSystem::imguize(vec2 v) {
    v *= imgui_scale;
    return {v.x, v.y};
}

void RenderSystem::drawMainMenu() {
    callback_system.in_hud = true;
    startDummyWindow("Full", {0, 0}, camera_system.camera_size);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    WorldPhase world_phase = registry.phases.components[0].phase;
    vec2 cursor_pos = camera_system.camera_size / 2.f;
    if (world_phase == WorldPhase::WELCOME) {
        cursor_pos.y /= 2.f;
        TextHeader text[] = {
                {"WAR!",                                                    true},
                {"The long centuries of peace in the galaxy are now over.", false},
                {"PREPARE TO FIGHT!",                                       false}
        };
        centreText(draw_list, text, cursor_pos);
    } else if (world_phase == WorldPhase::TUT1) {
        cursor_pos.y /= 2.f;
        TextHeader text[] = {
                {"Aim",                               true},
                {"To destroy all the enemy planets.", false},
                {"How",                               true},
                {
                 "Each turn starts with a planning phase where time is frozen.\nYou can launch your desired missile against other players.\nAfter you do, all entities will begin to move in the simulation phase",
                                                      false
                },
        };
        centreText(draw_list, text, cursor_pos);
    } else if (world_phase == WorldPhase::TUT2) {
        cursor_pos.y /= 2.f;
        TextHeader text[] = {
                {"Missile Selection",                                                            true},
                {"Use arrow keys to select between\n1) Normal\n2) Fast\n3) Cluster\n4) Gravity", false},
        };
        centreText(draw_list, text, cursor_pos);

    } else if (world_phase == WorldPhase::TUT3) {
        cursor_pos.y /= 2.f;
        TextHeader text[] = {
                {"Player List",                                  true},
                {"Check the life of every player on the right.", false},
        };
        centreText(draw_list, text, cursor_pos);
        cursor_pos.y *= 2.f;
        centreText(draw_list, "GET READY TO START!", cursor_pos, true);
    } else if (world_phase == WorldPhase::END) {
        for (int i = 0; i < registry.planets.components.size(); i++)
            if (registry.planets.components[i].life > 0.f) {
                std::string name = "Player" + std::to_string(i + 1);
                centreText(draw_list, name.c_str(), cursor_pos, true);
                break;
            }
    }


    ImGui::End();
}

void RenderSystem::drawGameHud() {
    callback_system.in_hud = false;
    ImColor hb_lost = {0.6f, 0.6f, 0.6f, 1.f};
    float health_bar_height = 20.f;
    float padding = 5.f;
    float spacing = 10.f;
    float line_height = ImGui::GetTextLineHeight() / imgui_scale;

    vec2 full_size = vec2(
            200.f + 2 * padding,
            line_height + 2 * padding + health_bar_height
    );
    vec2 pos = {
            camera_system.camera_size.x - 50.f - full_size.x,
            50.f
    };
    startDummyWindow("Text", pos, {full_size.x, 2 * (full_size.y + spacing)});
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    vec2 curr_pos = pos;
    for (int i = 0; i < registry.planets.components.size(); i++) {
        Planet p = registry.planets.components[i];

        ImColor bg_color = {0.5f, 0.5f, 0.8f, 1.f};
        float life_duration = 2000.f;
        float blink_duration = life_duration / 5;
        float displayed_life = p.old_life;
        if (p.time_since_change > 0.f) {
            float check = fmod(floor(p.time_since_change / blink_duration), 2.f);
            if (check == 0) bg_color = {0.8f, 0.5f, 0.5f, 1.f};
            displayed_life -= (p.time_since_change / life_duration) * (p.old_life - p.life);
        }


        float life_per = displayed_life / 100.f;

        vec2 max = curr_pos + full_size;
        draw_list->AddRectFilled(imguize(curr_pos), imguize(max), bg_color);

        curr_pos.y += padding;
        curr_pos.x += padding;
        ImGui::SetCursorPos(imguize(curr_pos - pos));
        std::string name = "Player" + std::to_string(i + 1);
        ImGui::Text(name.c_str());

        curr_pos.y += line_height + padding;
        max -= vec2(padding, padding);
        draw_list->AddRectFilled(imguize(curr_pos), imguize(max), hb_lost);

        ImColor color = {min(2.f - 2.f * life_per, 1.f), min(2.f * life_per, 1.f), 0.f, 1.f};
        max.x -= (1.f - life_per) * (full_size.x - 2.f * padding);
        draw_list->AddRectFilled(imguize(curr_pos), imguize(max), color);

        curr_pos += vec2(-padding, health_bar_height + padding + spacing);
    }
    ImGui::End();
}


void RenderSystem::updateVisibilityHudEntities() {
    WorldPhase world_phase = registry.phases.components[0].phase;
    for (int i = 0; i < registry.huds.components.size(); i++) {
        Entity e = registry.huds.entities[i];
        HUDComponent huds = registry.huds.components[i];
        bool in_phase = (huds.phases & world_phase) != world_phase;
        if (in_phase && !registry.hidden.has(e)) registry.hidden.emplace(e);
        if (!in_phase && registry.hidden.has(e)) registry.hidden.remove(e);
    }
}

void RenderSystem::drawGameOver() {
    startDummyWindow("Full", {0, 0}, camera_system.camera_size);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    vec2 cursor_pos = camera_system.camera_size / 2.f;

    for (int i = 0; i < registry.planets.components.size(); i++)
        if (registry.planets.components[i].life > 0.f) {
            std::string name = "Player" + std::to_string(i + 1) + " Wins!";
            centreText(draw_list, name.c_str(), cursor_pos, true);
            break;
        }


    ImGui::End();
}

void RenderSystem::drawHUD() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    WorldPhase world_phase = registry.phases.components[0].phase;
    if (world_phase == WorldPhase::GAME) drawGameHud();
    else {
        if (world_phase == WorldPhase::END) drawGameOver();
        else drawMainMenu();
        if (world_phase == WorldPhase::TUT3 || world_phase == WorldPhase::END) drawGameHud();
        startDummyWindow("Full", {0, 0}, camera_system.camera_size);
        vec2 cursor_pos = camera_system.camera_size / 2.f;
        cursor_pos.y *= 1.5f;
        centreText(ImGui::GetWindowDrawList(), "Press Spacebar to continue", cursor_pos);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderSystem::startDummyWindow(const char *name, vec2 position, vec2 size, float alpha) {
    bool open = true;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
                             | ImGuiWindowFlags_NoResize
                             | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoScrollbar
                             | ImGuiWindowFlags_NoSavedSettings
                             | ImGuiWindowFlags_NoInputs;

    ImGui::SetNextWindowBgAlpha(alpha);
    ImGui::Begin(name, &open, flags);
    ImGui::SetWindowPos(imguize(position));
    ImGui::SetWindowSize(imguize(size));
}

template<size_t N>
void RenderSystem::centreText(ImDrawList *draw_list, TextHeader (&text_arr)[N], vec2 pos) {
    float height = 0;
    float max_width = 0;
    float widths[N];
    float heights[N];
    int i = 0;
    bool is_header = false;
    for (TextHeader text: text_arr) {
        if (text.is_header && !is_header) ImGui::PushFont(header);
        if (!text.is_header && is_header) ImGui::PopFont();
        is_header = text.is_header;
        ImVec2 size = ImGui::CalcTextSize(text.text);
        widths[i] = size.x / imgui_scale;
        heights[i] = size.y / imgui_scale;
        max_width = max(max_width, widths[i]);
        height += heights[i];
        i++;
    }

    vec2 size = {max_width, height};
    drawRectangle(draw_list, pos - size / 2.f, size, {0.f, 0.f, 0.f, 0.5f}, {10.f, 5.f});
    pos.y -= height / 2;
    i = 0;
    for (TextHeader text: text_arr) {
        pos.x -= widths[i] / 2;
        if (text.is_header && !is_header) ImGui::PushFont(header);
        if (!text.is_header && is_header) ImGui::PopFont();
        is_header = text.is_header;
        ImGui::SetCursorPos(imguize(pos));
        ImGui::Text(text.text);
        pos.x += widths[i] / 2;
        pos.y += heights[i];
        i++;
    }
    if (is_header) ImGui::PopFont();
}

void RenderSystem::centreText(ImDrawList *draw_list, const char *text, vec2 cursor_pos, bool is_header) {
    TextHeader arr[1] = {{text, is_header}};
    centreText<1>(draw_list, arr, cursor_pos);
}

void RenderSystem::drawRectangle(ImDrawList *draw_list, vec2 position, vec2 size, vec4 color, vec2 padding) {
    draw_list->AddRectFilled(
            imguize(position - padding),
            imguize(position + size + padding),
            ImColor(color.x, color.y, color.z, color.w)
    );
}