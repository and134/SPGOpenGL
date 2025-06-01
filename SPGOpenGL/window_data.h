#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

void initWindows();

void initWindowShaders();

void loadWindowTextures();

void drawWindows(const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& viewPos,
    float timeOfDay,
    const glm::vec3& sunPosition,
    float sunIntensity);

void cleanupWindows();