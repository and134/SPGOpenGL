#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

// Ini?ializeazã geometria geamurilor ?i shader-ele
void initWindows();

// Ini?ializeazã shader-ele pentru geamuri
void initWindowShaders();

// Încarcã texturile pentru geamuri
void loadWindowTextures();

// Deseneazã geamurile cu efecte de timp ?i luminã
void drawWindows(const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& viewPos,
    float timeOfDay,
    const glm::vec3& sunPosition,
    float sunIntensity);

// Curã?ã resursele geamurilor
void cleanupWindows();