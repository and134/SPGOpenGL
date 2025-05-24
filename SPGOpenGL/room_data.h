#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

// Inițializează geometria camerei și texturile pereților, podelei și tavanului.
void initRoom(GLuint wallTex, GLuint wallNorm,
    GLuint floorTex, GLuint floorNorm,
    GLuint ceilTex, GLuint ceilNorm,
    GLuint shader);

// Desenează camera pe baza matricei de proiecție, view, pozițiile surselor de lumină,
// numărul lor, poziția camerei și forța efectului de normal mapping.
void drawRoom(const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3* lightPositions,
    int numLights,
    const glm::vec3& viewPos,
    float normalMapStrength, 
    float lightIntensiy);
