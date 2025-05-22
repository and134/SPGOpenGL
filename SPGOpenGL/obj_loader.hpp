#pragma once
#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

struct Mesh {
    GLuint vao, vbo, ebo;
    size_t indexCount;
};

Mesh loadOBJ(const std::string& path);
