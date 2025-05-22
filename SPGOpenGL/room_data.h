#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

// Inițializează geometria camerei (VAO, VBO, texturi etc.)
void initRoom(GLuint wallTex, GLuint wallNorm,
    GLuint floorTex, GLuint floorNorm,
    GLuint ceilTex, GLuint ceilNorm,
    GLuint shader);

// Desenează camera cu matrici de proiecție și view, poziție lumină și observator, și forța normal mapping
void drawRoom(const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& lightPos,
    const glm::vec3& viewPos,
    float normalMapStrength);
