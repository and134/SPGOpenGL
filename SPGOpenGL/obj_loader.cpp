#include "obj_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <map>

struct VertexKey {
    int pos, tex, norm;
    bool operator<(const VertexKey& other) const {
        if (pos != other.pos) return pos < other.pos;
        if (tex != other.tex) return tex < other.tex;
        return norm < other.norm;
    }
};

Mesh loadOBJ(const std::string& path) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLuint> indices;
    std::vector<float> vertexData;


    std::map<VertexKey, GLuint> vertexMap;
    GLuint vertexCount = 0;

    std::ifstream file(path);
    if (!file) {
        std::cerr << "Eroare la deschiderea modelului: " << path << std::endl;
        return {};
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

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
            normals.push_back(glm::normalize(n));
        }
        else if (type == "f") {
            std::vector<std::string> faceVertices;
            std::string vertex;
            while (ss >> vertex) {
                faceVertices.push_back(vertex);
            }

           
            std::vector<int> triangleIndices;
            if (faceVertices.size() == 3) {
                triangleIndices = { 0, 1, 2 };
            }
            else if (faceVertices.size() == 4) {
                triangleIndices = { 0, 1, 2, 0, 2, 3 }; 
            }

            for (int idx : triangleIndices) {
                std::string face = faceVertices[idx];
                std::replace(face.begin(), face.end(), '/', ' ');
                std::stringstream fss(face);

                int vi = 0, ti = 0, ni = 0;
                fss >> vi;
                if (fss.peek() != EOF) fss >> ti;
                if (fss.peek() != EOF) fss >> ni;

                if (vi > 0 && vi <= positions.size()) {
                    VertexKey key = { vi, ti, ni };

                    auto it = vertexMap.find(key);
                    if (it != vertexMap.end()) {
                       
                        indices.push_back(it->second);
                    }
                    else {
                       
                        glm::vec3 pos = positions[vi - 1];
                        glm::vec2 tex = (ti > 0 && ti <= texCoords.size()) ? texCoords[ti - 1] : glm::vec2(0.0f);
                        glm::vec3 norm = (ni > 0 && ni <= normals.size()) ? normals[ni - 1] : glm::vec3(0.0f, 1.0f, 0.0f);

                        vertexData.insert(vertexData.end(), {
                            pos.x, pos.y, pos.z,
                            norm.x, norm.y, norm.z,
                            tex.x, tex.y
                            });

                        vertexMap[key] = vertexCount;
                        indices.push_back(vertexCount);
                        vertexCount++;
                    }
                }
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

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    std::cout << "Loaded model: " << path << std::endl;
    std::cout << "Unique vertices: " << vertexCount << std::endl;
    std::cout << "Indices: " << indices.size() << std::endl;
    std::cout << "Triangles: " << indices.size() / 3 << std::endl;

    return mesh;
}