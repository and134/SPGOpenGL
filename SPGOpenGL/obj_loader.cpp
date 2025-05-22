#include "obj_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Mesh loadOBJ(const std::string& path) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLuint> indices;

    std::vector<float> vertexData;

    std::ifstream file(path);
    if (!file) {
        std::cerr << "Eroare la deschiderea modelului: " << path << std::endl;
        return {};
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;
        if (type == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (type == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            texCoords.push_back(uv);
        }
        else if (type == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (type == "f") {
            for (int i = 0; i < 3; ++i) {
                std::string face;
                ss >> face;
                std::replace(face.begin(), face.end(), '/', ' ');
                std::stringstream fss(face);
                int vi, ti, ni;
                fss >> vi >> ti >> ni;

                glm::vec3 v = positions[vi - 1];
                glm::vec2 uv = texCoords[ti - 1];
                glm::vec3 n = normals[ni - 1];

                vertexData.insert(vertexData.end(), { v.x, v.y, v.z, n.x, n.y, n.z, uv.x, uv.y });
                indices.push_back((GLuint)(indices.size()));
            }
        }
    }

    Mesh mesh;
    mesh.indexCount = indices.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);           // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // TexCoords
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    std::cout << "Loaded vertices: " << vertexData.size() / 8 << "\n";
    std::cout << "Loaded indices: " << indices.size() << "\n";
    return mesh;
}
