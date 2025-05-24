#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

void initRoom(GLuint wallTex, GLuint wallNorm,
    GLuint floorTex, GLuint floorNorm,
    GLuint ceilTex, GLuint ceilNorm,
    GLuint shader);

void drawRoom(const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3* lightPositions,
    int numLights,
    const glm::vec3& viewPos,
    float normalMapStrength, 
    float lightIntensiy);
