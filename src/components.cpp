#include "components.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include "../ext/stb_image/stb_image.h"

#include <iostream>

Debug debugging;
float death_timer_timer_ms = 3000;

bool Mesh::loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex> &out_vertices,
                           std::vector<uint16_t> &out_vertex_indices, vec2 &out_size) {
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

    printf("Loading OBJ file %s...\n", obj_path.c_str());
    std::vector<uint16_t> out_uv_indices, out_normal_indices;
    std::vector<glm::vec2> out_uvs;
    std::vector<glm::vec3> out_normals;

    FILE *file = fopen(obj_path.c_str(), "r");
    if (file == NULL) {
        printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
        getchar();
        return false;
    }

    while (1) {
        char lineHeader[128];
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break;

        if (strcmp(lineHeader, "v") == 0) {
            auto vertex = ColoredVertex({0, 0, 0}, {0, 0, 0});
            int matches = fscanf(file, "%f %f %f %f %f %f\n", &vertex.position.x, &vertex.position.y,
                                 &vertex.position.z,
                                 &vertex.color.x, &vertex.color.y, &vertex.color.z);
            if (matches == 3)
                vertex.color = {1, 1, 1};
            out_vertices.push_back(vertex);
        } else if (strcmp(lineHeader, "vt") == 0) {
            glm::vec2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y);
            uv.y = -uv.y;
            out_uvs.push_back(uv);
        } else if (strcmp(lineHeader, "vn") == 0) {
            glm::vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            out_normals.push_back(normal);
        } else if (strcmp(lineHeader, "f") == 0) {
            std::string vertex1, vertex2, vertex3;
            unsigned int vertexIndex[3], normalIndex[3], uvIndex[3];

            int matches = fscanf(file, "%d %d %d\n", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2]);
            if (matches == 1) {
                matches = fscanf(file, "//%d %d//%d %d//%d\n", &normalIndex[0], &vertexIndex[1], &normalIndex[1],
                                 &vertexIndex[2], &normalIndex[2]);
                if (matches != 5) {
                    matches = fscanf(file, "%d/%d %d/%d/%d %d/%d/%d\n", &uvIndex[0], &normalIndex[0], &vertexIndex[1],
                                     &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
                    if (matches != 8) {
                        printf("File can't be read by our simple parser :-( Try exporting with other options\n");
                        fclose(file);
                        return false;
                    }
                }
            }
            out_vertex_indices.push_back((uint16_t) vertexIndex[0] - 1);
            out_vertex_indices.push_back((uint16_t) vertexIndex[1] - 1);
            out_vertex_indices.push_back((uint16_t) vertexIndex[2] - 1);
            out_normal_indices.push_back((uint16_t) normalIndex[0] - 1);
            out_normal_indices.push_back((uint16_t) normalIndex[1] - 1);
            out_normal_indices.push_back((uint16_t) normalIndex[2] - 1);
        } else {
            char stupidBuffer[1000];
            fgets(stupidBuffer, 1000, file);
        }
    }
    fclose(file);
    vec3 max_position = {-99999, -99999, -99999};
    vec3 min_position = {99999, 99999, 99999};
    for (ColoredVertex &pos: out_vertices) {
        max_position = glm::max(max_position, pos.position);
        min_position = glm::min(min_position, pos.position);
    }
    if (abs(max_position.z - min_position.z) < 0.001)
        max_position.z = min_position.z + 1;

    vec3 size3d = max_position - min_position;
    out_size = size3d;

    for (ColoredVertex &pos: out_vertices)
        pos.position = ((pos.position - min_position) / size3d) - vec3(0.5f, 0.5f, 0.5f);

    return true;
}
