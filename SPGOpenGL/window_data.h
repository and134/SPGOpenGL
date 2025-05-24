#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

// Ini?ializeaz� geometria geamurilor ?i shader-ele
void initWindows();

// Ini?ializeaz� shader-ele pentru geamuri
void initWindowShaders();

// �ncarc� texturile pentru geamuri
void loadWindowTextures();

// Deseneaz� geamurile cu efecte de timp ?i lumin�
void drawWindows(const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& viewPos,
    float timeOfDay,
    const glm::vec3& sunPosition,
    float sunIntensity);

// Cur�?� resursele geamurilor
void cleanupWindows();